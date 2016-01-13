#ifndef AK_STAND_ALONE_H
#define AK_STAND_ALONE_H

#define AKER_PROF_PORT		1022	/* Porta UDP do protocolo Aker-prof */
#define AKER_PROF_VERSION	2	/* Versao do protocolo Aker-prof */
#define APROF_BIND_PORT		18	/* Associa usuario com porta na maquina onde esta logado*/

typedef struct
{
  struct in_addr ip_src;		///< IP origem do usuario da sessao
  aku32 seq;				///< Contador sequencial
  aku32 user_num;			///< Numero serial do usuario
  aku16 port;				///< Numero da porta origem
  aku8 protocol;			/// IPPROTO_TCP ou IPPROTO_UDP
  aku8 reserved;			///< Reservado. Deve ser 0
  char hash[16];			///< Autenticacao MD5(MD5(segredo) + seq + user_num + port + protocol + reserved)
} fwprofd_port_ctl;

typedef struct	fwprofd_header_ /* Header presente em todos os pacotes */
{
  u_char versao;		/* Versao do protocolo, ver define */
  u_char tipo_req;		/* Tipo de requisicao, ver defines */
  struct in_addr ip_dst;	/* IP destino p/ o qual o pacote foi enviado */ 
  unsigned char md5[16];	/* MD5 do pacote */
} fwprofd_header;

#endif // AK_STAND_ALONE_H
