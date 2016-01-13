/*!
 * \file ak_client_hook.c
 * \brief Modulo de interceptacao de pacotes do Aker Client.
 * \date 05/09/2007
 * \author Rodrigo Ormonde
 * \ingroup akerclient_driver_linux
 *
 * Copyright (c)1997-2007 Aker Security Solutions
 *
 * $Id: ak_client_hook.c,v 1.15.2.10 2013/02/18 13:52:24 artur.soares Exp $
 */

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/netfilter_ipv4.h>
#include <linux/module.h>
#include <net/route.h>
#include <asm/checksum.h>

#include "aktypes.h"
#include "ak_client_drv.h"
#ifdef STAND_ALONE
# include "ak_stand_alone.h"
#else
# include "fwprofd.h"
#endif
#include "md5.h"

//#define DEBUG_HOOK

// Macros que nao existem em alguns kernels 2.4

#ifndef HH_DATA_MOD
#define HH_DATA_MOD     16
#endif

#ifndef HH_DATA_OFF
#define HH_DATA_OFF(__len) \
       (HH_DATA_MOD - ((__len) & (HH_DATA_MOD - 1)))
#endif

#ifndef HH_DATA_ALIGN
#define HH_DATA_ALIGN(__len) \
        (((__len)+(HH_DATA_MOD-1))&~(HH_DATA_MOD - 1))
#endif

#ifdef DEBUG_HOOK
#  define PRINT(x, y...) printk(x, ## y)
#  warning **************************
#  warning **** Debug Habilitado ****
#  warning **************************

static char *ip2a(aku32 ip)
{
  static int nOut=0;
  static char outs[4][16];
  char *out = outs[(nOut++) & 3];
  sprintf(out,"%d.%d.%d.%d", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff,(ip >> 24) & 0xff);

  return out;
}
#else
#  define PRINT(y...) {}
#endif		// DEBUG_HOOK

#if ((LINUX_VERSION_CODE < KERNEL_VERSION(2,6,43)) || \
    ((LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)) && \
    (LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0))))
# define dst_get_neighbour_noref dst_get_neighbour
#endif 

// Prototipo das funcoes locais
//
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0))
u_int ak_client_out_chk(u_int hooknum,
#else
u_int ak_client_out_chk(const struct nf_hook_ops *ops,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
                        struct sk_buff **pskb,
#else
                        struct sk_buff *skb,
#endif
                        const struct net_device *dev,
                        const struct net_device *out_dev,
                        int (*okfn)(struct sk_buff *));
// Variaveis globais

/** spinlock de acesso tabela de usuarios logados */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
static spinlock_t ak_client_spinlock = SPIN_LOCK_UNLOCKED;
#else
static DEFINE_SPINLOCK(ak_client_spinlock);
#endif
static ak_client_logon_array logon_array[AK_CLIENT_MAX_LOGGED_USERS];
static int logon_array_size = 0;

static struct nf_hook_ops ak_client_hook_out = {
  .hook     = ak_client_out_chk,
  .owner    = THIS_MODULE,
  .pf       = PF_INET,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
  .hooknum  = NF_IP_LOCAL_OUT,
#else
  .hooknum  = NF_INET_LOCAL_OUT,
#endif
  .priority = NF_IP_PRI_LAST,
};

static int udp_enable = 1;		// Intercepta pacotes UDP ?

/** spinlock de acesso tabela de pacotes a enviar */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
static spinlock_t ak_client_queue_spinlock = SPIN_LOCK_UNLOCKED;
#else
static DEFINE_SPINLOCK(ak_client_queue_spinlock);
#endif
static ak_send_queue *queue_first = NULL;	// Primeiro pacote a enviar
static ak_send_queue *queue_last = NULL;	// Ultimo pacote a enviar
static struct timer_list ak_client_timer;
static int timer_shutdown = 0;			// Modulo sendo removido ?


/*! Esta funcao realiza uma pesquisa binaria na tabela de usuarios logados
    para encontrar o par usuario/IP

    \param uid Usuario a ser verificado/incluido na tabela
    \param ip Firewall no qual o usuario a ser verificado/incluido logou
    \param pos Parametro de output que indica a posicao onde o par usuario/IP
               se encontra ou onde deve ser inserido (de acordo com o valor de
               retorno da funcao)

    \warning Esta funcao deve ser chamada com o spinlock ak_client_spinlock lockado

    \return 1 Usuario/IP encontrados
    \return 0 Usuario/IP inexistente
*/

static __inline int ak_client_search_user(unsigned int uid, struct in_addr ip, int *pos)
{
  int i = 0, j, k = 0;

  j = logon_array_size - 1;
  while (i <= j)
  {
    k = (i + j) >> 1;
    if (uid > logon_array[k].logon_data.unix_user_num)
      i = k + 1;
    else if (uid < logon_array[k].logon_data.unix_user_num)
      j = k - 1;
    else		      // uid igual. Compara IPs
    {
      if (ip.s_addr > logon_array[k].logon_data.ip.s_addr)
        i = k + 1;
      else if (ip.s_addr < logon_array[k].logon_data.ip.s_addr)
        j = k - 1;
      else			// Achou, retorna a posicao
      {
        *pos = k;
        return 1;
      }
    }
  }
  *pos = k;

  if (logon_array_size)
  {
    if (uid > logon_array[k].logon_data.unix_user_num)
     (*pos)++;
    else if ((uid == logon_array[k].logon_data.unix_user_num) &&
             (ip.s_addr > logon_array[k].logon_data.ip.s_addr))
     (*pos)++;
  }
  return 0;			// Nao achou, retorna onde inserir
}


/*! Esta funcao monta uma lista com todos os firewalls nos quais o usuario
    informado efetuou logon

    \param uid Usuario a ser verificado
    \param user_logon Ponteiro para um buffer onde a lista de firewalls nos
                      quais o usuario logou sera copiada

    \note O numero maximo de entradas que serao copiadas para user_logon (e o
          tamanho minimo deste buffer): #AK_CLIENT_MAX_LOGONS_PER_USER

    \return Numero de entradas retornadas em user_logon
*/

static int ak_client_get_user_list(unsigned int uid, ak_client_logon_array *user_logon)
{
  int pos, pos2;
  struct in_addr ip = {.s_addr = 0 };
  int count;

  spin_lock_bh(&ak_client_spinlock);
  ak_client_search_user(uid, ip, &pos);

  if (logon_array[pos].logon_data.unix_user_num != uid)
  {
    spin_unlock_bh(&ak_client_spinlock);
    PRINT("Nao encontrei dados de logon do usuario %u\n", uid);
    return 0;
  }

  // Conta quantas entradas existem para este usuario

  pos2 = pos;
  while ((pos2 < logon_array_size) && (logon_array[pos2].logon_data.unix_user_num == uid))
  {
    logon_array[pos2].seq++;
    pos2++;
  }
  count = pos2 - pos;
  PRINT("Retornando %d estruturas de logon do usuario %u\n", count, uid);
  memcpy(user_logon, &logon_array[pos], count * sizeof(ak_client_logon_array));
  spin_unlock_bh(&ak_client_spinlock);

  return count;
}


/*! Esta funcao remove informacao de todos os usuarios logados */

void ak_client_flush_users(void)
{
  spin_lock_bh(&ak_client_spinlock);
  logon_array_size = 0;
  spin_unlock_bh(&ak_client_spinlock);
}


/*! Esta funcao habilita ou desabilida a captura de pacotes UDP pelo hook

    \param enable Se TRUE habilita captura de pacotes UDP pelo hook
*/

void ak_client_udp_enable(int enable)
{
  udp_enable = enable;
}


/*! Esta funcao acrescenta um novo usuario no array de usuarios logados

    \param logon Dados do usuario que efetuou o logon

    \return 0 OK
    \return !=0 Erro. Ver errno.h
*/

int ak_client_add_user(ak_client_logon *logon_data)
{
  MD5_CTX contexto;			// Contexto para calcular MD5
  int pos;

  // Faz o hash do secret, como e' usado na comunicacao com o fwprofd

  MD5Init(&contexto);
  MD5Update(&contexto, (u_char *) logon_data->secret, 16);
  MD5Final(logon_data->secret, &contexto);

  // Inclui usuario

  PRINT("Incluindo logon do usuario %u\n", logon_data->unix_user_num);
  spin_lock_bh(&ak_client_spinlock);
  if (!ak_client_search_user(logon_data->unix_user_num, logon_data->ip, &pos))	// Usuario ta logado ?
  {
    int pos2;
    int n_logons;		// Numero de logons de um mesmo usuario;

    if (logon_array_size >= AK_CLIENT_MAX_LOGGED_USERS)
    {
      spin_unlock_bh(&ak_client_spinlock);
      PRINT("Erro ao incluir: mais de %d usuarios logados\n", AK_CLIENT_MAX_LOGGED_USERS);
      return E2BIG;
    }

    // Verifica o numero maximo de logons por usuario
    // Volta ate a primeira ocorrencia DESSE uid no array de usuarios logados
    for (pos2 = pos; (pos2 > 0) && (logon_array[pos2 - 1].logon_data.unix_user_num == logon_data->unix_user_num); )
    {
      pos2--;
    }
    // Avanca ate a ultima ocorrencia desse uid no array de usuarios logados, incrementando n_logons
    for (n_logons = 0; (pos2 < logon_array_size) && (logon_array[pos2].logon_data.unix_user_num == logon_data->unix_user_num); )
    {
      pos2++;
      n_logons++;
    }

    if (n_logons >= AK_CLIENT_MAX_LOGONS_PER_USER)
    {
      spin_unlock_bh(&ak_client_spinlock);
      PRINT("Erro ao incluir: mais de %d logons para o usuario %u\n",
            AK_CLIENT_MAX_LOGONS_PER_USER, logon_data->unix_user_num);
      return E2BIG;
    }

    // Se vai incluir no meio do array, abre espaco para um elemento com memmove
    memmove(&logon_array[pos + 1], &logon_array[pos], (logon_array_size - pos) * sizeof(ak_client_logon_array));
    logon_array_size++;
  }
  memmove(&logon_array[pos].logon_data, logon_data, sizeof(ak_client_logon_array));
  logon_array[pos].seq = 0;
  spin_unlock_bh(&ak_client_spinlock);

  return 0;
}


/*! Esta funcao remove um usuario do array de usuarios logados

    \param uid Userid do usuario a ser removido

    \return 0 OK
    \return !=0 Erro. Ver errno.h
*/

int ak_client_remove_user(ak_client_logoff *logoff_data)
{
  int pos;

  spin_lock_bh(&ak_client_spinlock);
  if (!ak_client_search_user(logoff_data->unix_user_num, logoff_data->ip, &pos))
  {
    // Usuario nao estava logado ?!?!?!

    spin_unlock_bh(&ak_client_spinlock);
    return ENOENT;
  }
  logon_array_size--;
  memmove(&logon_array[pos], &logon_array[pos + 1], (logon_array_size - pos) * sizeof(ak_client_logon_array));
  spin_unlock_bh(&ak_client_spinlock);

  return 0;
}


/*! Esta funcao informa ao firewall (ou outro produto no qual o cliente esta
    logado) que uma determinada porta esta agora associada a um novo usuario

    \param port_src Porta origem que sera associada a um usuario
    \param protocol Protocolo da porta: IPPROTO_TCP ou IPPROTO_UDP
    \param uid Userid do usuario que esta enviando o pacote

    \note Se um usuario estiver logado em varios firewalls, sera enviado um
          pacote para cada um deles

    \return 0 Pelo menos um pacote foi enviado a um produto com sucesso
    \return <0 Erro ao enviar pacote ou nao havia nenhum usuario logado
*/

static int ak_client_inform_port(const struct net_device *dev, aku16 port_src,
                                 aku8 protocol, unsigned int uid)
{
  ak_client_logon_array user_logon[AK_CLIENT_MAX_LOGONS_PER_USER];
  struct sk_buff *skb;			// Pacote a ser enviado para avisar o firewall
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
  struct flowi flp;
#else
  struct flowi4 flp;
#endif
  struct in_device *idev;
  struct rtable *rt;			// Rota a ser usada para enviar o pacote
  struct iphdr *ip;			// Header IP do pacote a enviar
  struct udphdr *udp;			// Header UDP do pacote a enviar
  struct dst_entry *dst;
#if (((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,41)) && \
    (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))) || \
    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)))
  struct neighbour *neigh;
#endif
  MD5_CTX contexto;			// Contexto para calcular MD5
  int pkt_sent = 0;			// Enviou ao menos um pacote ?
  fwprofd_header *header;
  fwprofd_port_ctl *port_ctl;
  ak_client_logon_array *logon;
  int size;
  int count;
  int i;

  if (!dev)
  {
    PRINT("Device de saida NULL\n");
    return -2;
  }
  count = ak_client_get_user_list(uid, user_logon);
  size = sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(fwprofd_header) + sizeof(fwprofd_port_ctl);

  for (i = 0, logon = user_logon; i < count; i++, logon++)
  {
    PRINT("Enviando pacote %d/%d - ", i + 1, count);

    skb = alloc_skb(size + 16, GFP_ATOMIC);
    if (!skb)
    {
      PRINT("Nao consegui alocar skbuff para enviar pacote\n");
      return -3;
    }
    skb->data += 16;
    skb->len = size;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
    skb->tail = skb->data + size;
    skb->nh.iph = (struct iphdr *) skb->data;
    skb->h.uh = (struct udphdr *) (skb->data + sizeof(struct iphdr));
    ip = skb->nh.iph;
#else
    skb_set_tail_pointer(skb, size);
    skb_reset_network_header(skb);
    skb_set_transport_header(skb, sizeof(struct iphdr));
    ip = ip_hdr(skb);
#endif
    udp = (struct udphdr *) ((char *) ip + sizeof(struct iphdr));
    header = (fwprofd_header *) (udp + 1);
    port_ctl = (fwprofd_port_ctl *) (header + 1);

    // Pega o IP da interface de saida para alocar rota de saida

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    idev = in_dev_get(dev);
#else
    rcu_read_lock();
    idev = __in_dev_get_rcu(dev);
#endif

    if (!idev)
    {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
      rcu_read_unlock();
#endif
      kfree_skb(skb);
      PRINT("Device de saida sem IP (1)\n");
      return -4;
    }
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    read_lock(&idev->lock);
#endif

    if (!idev->ifa_list)
    {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
      read_unlock(&idev->lock);
      in_dev_put(idev);
#else
      rcu_read_unlock();
#endif
      kfree_skb(skb);
      PRINT("Device de saida sem IP (2)\n");
      return -5;
    }
    ip->saddr = idev->ifa_list->ifa_address;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0))
    read_unlock(&idev->lock);
    in_dev_put(idev);
#else
    rcu_read_unlock();
#endif


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
    flp.oif = 0;
    flp.nl_u.ip4_u.saddr = ip->saddr;
    flp.nl_u.ip4_u.daddr = logon->logon_data.ip.s_addr;
    flp.nl_u.ip4_u.tos = 0;
    flp.uli_u.ports.sport = ntohs(AKER_PROF_PORT);
    flp.uli_u.ports.dport = ntohs(AKER_PROF_PORT);
    flp.proto = IPPROTO_UDP;
#else
    flp.flowi4_oif = 0;
    flp.saddr = ip->saddr;
    flp.daddr = logon->logon_data.ip.s_addr;
    flp.flowi4_tos = 0;
    flp.fl4_sport = ntohs(AKER_PROF_PORT);
    flp.fl4_dport = ntohs(AKER_PROF_PORT);
    flp.flowi4_proto = IPPROTO_UDP;
#endif


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,39))
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,25))
    if (ip_route_output_key(&rt, &flp))
#else
    if (ip_route_output_key(&init_net, &rt, &flp))
#endif
    {
      kfree_skb(skb);
      PRINT("Erro ao alocar rota de saida\n");
      continue;
    }
#else
    rt = ip_route_output_key(&init_net, &flp);
    if (IS_ERR(rt))
    {
      kfree_skb(skb);
      PRINT("Erro ao alocar rota de saida\n");
      continue;
    }
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
    skb->dst = dst_clone(&rt->u.dst);
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
    skb_dst_set(skb, dst_clone(&rt->u.dst));
#else
    skb_dst_set(skb, dst_clone(&rt->dst));
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
    skb->dev = rt->u.dst.dev;
#else
    skb->dev = rt->dst.dev;
#endif
    skb->protocol = __constant_htons(ETH_P_IP);

    // Preenche dados do usuario

    port_ctl->ip_src.s_addr = 0;
    port_ctl->seq = ntohl(logon->seq);		// ak_client_get_user_list() ja incrementou seq
    port_ctl->user_num = ntohl(logon->logon_data.ak_user_num);
    port_ctl->port = port_src;
    port_ctl->protocol = protocol;
    port_ctl->reserved = 0;

    MD5Init(&contexto);
    MD5Update(&contexto, (u_char *) logon->logon_data.secret, 16);
    MD5Update(&contexto, (u_char *) &port_ctl->ip_src, sizeof(struct in_addr));
    MD5Update(&contexto, (u_char *) &port_ctl->seq, sizeof(aku32));
    MD5Update(&contexto, (u_char *) &port_ctl->user_num, sizeof(aku32));
    MD5Update(&contexto, (u_char *) &port_ctl->port, sizeof(aku16));
    MD5Update(&contexto, (u_char *) &port_ctl->protocol, sizeof(aku8));
    MD5Update(&contexto, (u_char *) &port_ctl->reserved, sizeof(aku8));
    MD5Final((u_char *) port_ctl->hash, &contexto);

    // Preenche demais campos do pacote

    header->ip_dst = logon->logon_data.ip;
    header->versao = AKER_PROF_VERSION;
    header->tipo_req = APROF_BIND_PORT;
    memset(header->md5, 0, 16);

    MD5Init(&contexto);
    MD5Update(&contexto, (void *) header, sizeof(fwprofd_header));
    MD5Update(&contexto, (void *) port_ctl, sizeof(fwprofd_port_ctl));
    MD5Final(header->md5, &contexto);

    udp->dest = udp->source = ntohs(AKER_PROF_PORT);
    udp->len = ntohs(size - sizeof(struct iphdr));
    udp->check = 0;

    ip->ihl = sizeof(struct iphdr) >> 2;
    ip->version = IPVERSION;
    ip->ttl = IPDEFTTL;
    ip->tos = 0;
    ip->daddr = header->ip_dst.s_addr;
    ip->protocol = IPPROTO_UDP;
    ip->frag_off = 0;
    ip->tot_len = htons(size);
    ip->id = 0;
    ip->check = 0;
    ip->check = ip_fast_csum((u_char *) ip, ip->ihl);
    PRINT("%s -> %s\n", ip2a(ip->saddr), ip2a(ip->daddr));

    // Envia pacote
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
    dst = skb->dst;
#else
    dst = skb_dst(skb);
#endif

#if (((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,41)) && \
    (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))) || \
    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)) && \
    LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))

    rcu_read_lock();
    neigh = dst_get_neighbour_noref(dst);
    
    if (neigh)
    {
      neigh->output(neigh, skb);
      ip_rt_put(rt);
      pkt_sent++;
    }

#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0))
    rcu_read_lock();
    neigh = dst_neigh_lookup_skb(dst, skb);

    if (neigh)
    {
      neigh->output(neigh, skb);
      ip_rt_put(rt);
      pkt_sent++;
    }
#else
    if (dst->hh)
    {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18))
      int hh_alen;

      read_lock_bh(dst->hh->hh_lock);
      hh_alen = HH_DATA_ALIGN(dst->hh->hh_len);
      memcpy(skb->data - hh_alen, dst->hh->hh_data, hh_alen);
      read_unlock_bh(dst->hh->hh_lock);
      skb_push(skb, dst->hh->hh_len);
      dst->hh->hh_output(skb);
#else
      neigh_hh_output(dst->hh, skb);
#endif
      ip_rt_put(rt);
      pkt_sent++;
    }
    else if (dst->neighbour)
    {
      dst->neighbour->output(skb);
      ip_rt_put(rt);
      pkt_sent++;
    }
#endif
    else
    {
      kfree_skb(skb);
      ip_rt_put(rt);
      PRINT("Nao sei como enviar pacote de saida\n");
    }

#if (((LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,41)) && \
    (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))) || \
    (LINUX_VERSION_CODE >= KERNEL_VERSION(3,1,0)))
    rcu_read_unlock();
#endif
  }
  if (!pkt_sent)
    return -1;

  return 0;
}


/*! Esta funcao e' chamada periodicamente por um timer e serve para enviar os
    pacotes que estao pendentes na fila de envio */

void ak_client_timer_func(unsigned long unused)
{
  ak_send_queue *aux_queue;

  spin_lock_bh(&ak_client_queue_spinlock);
  while (queue_first)
  {
    if (queue_first->jiffies > jiffies)
      break;

    aux_queue = queue_first;
    queue_first = queue_first->next;
    if (!queue_first)
      queue_last = NULL;

    PRINT("timer_func: enviando pacote original %s -> %s\n", 
           ip2a(ip_hdr(aux_queue->skb)->saddr), ip2a(ip_hdr(aux_queue->skb)->daddr));

    (*aux_queue->okfn)(aux_queue->skb);
    kfree(aux_queue);
  }

  if (queue_first && !timer_shutdown)
  {
    init_timer(&ak_client_timer);
    ak_client_timer.expires = queue_first->jiffies;
    ak_client_timer.function = ak_client_timer_func;
    add_timer(&ak_client_timer);
  }
  spin_unlock_bh(&ak_client_queue_spinlock);
}


/*! Esta funcao intercepta pacotes gerados na propria maquina para informar 
    ao produto no qual o cliente esta logado que uma nova conexao esta sendo
    criada.
*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0))
u_int ak_client_out_chk(u_int hooknum,
#else
u_int ak_client_out_chk(const struct nf_hook_ops *ops,
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
                        struct sk_buff **pskb,
#else
                        struct sk_buff *skb,
#endif
                        const struct net_device *dev,
                        const struct net_device *out_dev,
                        int (*okfn)(struct sk_buff *))
{
  struct iphdr *ip;
  int process_pkt = 0;		// Deve processar pacote ?
  aku16 port_src;		// Porta origem do pacote, se processado
  unsigned int uid;		// Userid de quem esta enviando o pacote

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
  struct sk_buff **pskb = &skb;
#endif

  // Verifica se pacote e relevante

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
  ip = (*pskb)->nh.iph;
#else
  ip = ip_hdr(*pskb);
#endif

  if (ip->protocol == IPPROTO_TCP)
  {
    struct tcphdr *tcp;

    tcp = (struct tcphdr *) (((aku32 *) (ip)) + ip->ihl);
    if (tcp->syn && !tcp->ack)
    {
      process_pkt = 1;
      port_src = tcp->source;
    }
  }
  else if ((ip->protocol == IPPROTO_UDP) && udp_enable)
  {
    struct udphdr *udp;

    udp = (struct udphdr *) (((aku32 *) (ip)) + ip->ihl);
    process_pkt = 1;
    port_src = udp->source;
  }

  if (!process_pkt)
    return NF_ACCEPT;

  if (!(*pskb)->sk || !(*pskb)->sk->sk_socket || !(*pskb)->sk->sk_socket->file)
  {
    PRINT("Nao encontrei usuario. prot = %hu, port_src = %hu\n", ip->protocol, ntohs(port_src));
    return NF_ACCEPT;
  }

  // Pacote deve ser processado. Extrai informacoes do usuario do socket

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29))
  uid = (*pskb)->sk->sk_socket->file->f_uid;
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3,5,0))
  uid = (*pskb)->sk->sk_socket->file->f_cred->uid;
#else
  uid = from_kuid(&init_user_ns, (*pskb)->sk->sk_socket->file->f_cred->uid);
#endif

  PRINT("Processando. prot = %hu, port_src = %hu, user = %u\n", ip->protocol, ntohs(port_src), uid);
  if (!ak_client_inform_port(out_dev, port_src, ip->protocol, uid))
  {
    ak_send_queue *aux_queue;

    // Firewall foi informado. Coloca pacote na fila para ser enviado dentro
    // de alguns instantes, para dar tempo ao firewall de processa-lo

    aux_queue = (ak_send_queue *) kmalloc(sizeof(ak_send_queue), GFP_ATOMIC);
    if (!aux_queue)
    {
      PRINT("Nao consegui alocar memoria para a fila\n");
      return NF_ACCEPT;
    }
    aux_queue->next = NULL;
    aux_queue->okfn = okfn;
    aux_queue->skb = *pskb;
    aux_queue->jiffies = jiffies + AK_CLIENT_SEND_DELAY * HZ / 1000;

    spin_lock_bh(&ak_client_queue_spinlock);
    if (!queue_last)
    {
      // Fila vazia. Acrescenta pacote e dispara timer

      queue_last = queue_first = aux_queue;
      init_timer(&ak_client_timer);
      ak_client_timer.expires = aux_queue->jiffies;
      ak_client_timer.function = ak_client_timer_func;
      add_timer(&ak_client_timer);
    }
    else
    {
      queue_last->next = aux_queue;
      queue_last = aux_queue;
    }
    spin_unlock_bh(&ak_client_queue_spinlock);

    return NF_STOLEN;
  }
  return NF_ACCEPT;
}


/*! Esta funcao registra o hook de captura de pacotes no kernel do linux

    \return 0 OK
    \return -1 Erro ao registrar hook
*/

int ak_client_register_hook(void)
{
  init_timer(&ak_client_timer);
  if (nf_register_hook(&ak_client_hook_out))
    return -1;

  return 0;
}


/*! Esta funcao desregistra o hook de captura de pacotes no kernel do linux */

void ak_client_unregister_hook(void)
{
  ak_send_queue *aux_queue;

  timer_shutdown = 1;
  nf_unregister_hook(&ak_client_hook_out);
  spin_lock_bh(&ak_client_queue_spinlock);
  while (queue_first)
  {
    aux_queue = queue_first;
    queue_first = queue_first->next;
    kfree_skb(aux_queue->skb);
    kfree(aux_queue);
  }
  queue_last = NULL;
  spin_unlock_bh(&ak_client_queue_spinlock);
  del_timer_sync(&ak_client_timer);
}
