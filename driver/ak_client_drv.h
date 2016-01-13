/*!
 * \file ak_client_drv.h
 * \brief Definicoes para o driver de kernel Linux.
 * \date 05/09/2007
 * \author Rodrigo Ormonde
 * \ingroup akerclient_driver_linux
 *
 * Copyright (c)1997-2007 Aker Security Solutions
 *
 * $Id: ak_client_drv.h,v 1.4.2.1 2012/07/27 13:49:35 neuton.martins Exp $
 */

#ifndef _AK_CLIENT_DRV_H
#define _AK_CLIENT_DRV_H

/*!
 * \addtogroup akerclient
 * @{
 *
 * \defgroup akerclient_driver Driver para autenticacao por usuario
 *
 * Driver usados pelo Aker Client para fornecer a funcionalidade de protocolo
 * por usuario.
 * Mais informacoes sobre o protocolo por usuario podem ser encontrada aqui
 * \ref usuario_aut .
 *
 * \addtogroup akerclient_driver
 * @{
 *
 * \defgroup akerclient_driver_linux Linux
 *
 * Implementacao do driver que da suporte ao protocolo por usuario Linux.
 * No Linux esse driver e' um modulo de kernel que intercepta todos os pacotes
 * e trata eles de forma apropriada.
 *
 * \addtogroup akerclient_driver_linux
 * @{
 *
 */

// Definicoes gerais

#define AK_CLIENT_NAME			"aker_client"
#define AK_CLIENT_DEV_NAME		"/dev/aker_client"
#define AK_CLIENT_API_MINOR		0

/*! Numero maximo de usuarios que podem estar logados nesta maquina */
#define AK_CLIENT_MAX_LOGGED_USERS	256
/*! Numero maximo de firewalls nos quais um mesmo usuario pode se logar */
#define AK_CLIENT_MAX_LOGONS_PER_USER	12
/*! Delay entre o pacote de informacao ao firewall e o de abertura de conexao */
#define AK_CLIENT_SEND_DELAY		300

/*! Esta estrutura e' usada para informar o modulo do kernel que um usuario fez
    login em um dos produtos que suportam o Aker Client */

typedef struct
{
  unsigned int unix_user_num;	///< Numero do usuario no Linux
  unsigned int ak_user_num;	///< Numero do usuario como conhecido pelos produtos Aker
  struct in_addr ip;		///< Endereco IP do produto Aker onde usuario logou
  unsigned char secret[16];	///< Segredo compartilhado com o produto Aker
} ak_client_logon;

/*! Esta estrutura e' usada para informar o modulo do kernel que um usuario fez
    logout de um dos produtos que suportam o Aker Client */

typedef struct
{
  unsigned int unix_user_num;	///< Numero do usuario no Linux
  struct in_addr ip;		///< Endereco IP do produto Aker onde usuario logou
} ak_client_logoff;


// Operacoes de ioctl() no device

#define AK_CLIENT_LOGIN		_IOW ('A', 0, ak_client_logon)
#define AK_CLIENT_LOGOUT	_IOW ('A', 1, ak_client_logoff)
#define AK_CLIENT_FLUSH		_IO  ('A', 2)
#define AK_CLIENT_UDP_ENABLE	_IOW ('A', 3, int)

#ifdef __KERNEL__

/*! Esta estrutura representa a lista de usuarios logados na maquina */

typedef struct
{
  ak_client_logon logon_data;	///< Dados do usuario
  aku32 seq;			///< Numero de sequencia para enviar pacote
} ak_client_logon_array;


/*! Esta estrutura representa a lista de pacotes a serem enviados pela rede em
    um determinado momento */

typedef struct _ak_send_queue
{
  struct sk_buff *skb;			///< Ponteiro para o pacote a ser enviado
  int (*okfn)(struct sk_buff *);	///< Ponteiro para a funcao de envio
  unsigned long jiffies;		///< Quando enviar
  struct _ak_send_queue *next;		///< Proximo pacote da fila
} ak_send_queue;

// Prototipo das funcoes globais

int ak_client_register_hook(void);
void ak_client_unregister_hook(void);
void ak_client_flush_users(void);
int ak_client_add_user(ak_client_logon *);
int ak_client_remove_user(ak_client_logoff *);
void ak_client_udp_enable(int);

#endif		//__KERNEL__

/*!
 * @}
 * @}
 * @}
 */

#endif 		//_AK_CLIENT_DRV_H
