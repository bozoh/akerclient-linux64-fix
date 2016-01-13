/* aktypes.h - Definicoes de tipos multiplataforma dos programas Aker

22/01/2002 Criacao					Rodrigo Ormonde

	   Versao 5
	   $Id: aktypes.h,v 1.17 2012/10/11 19:31:48 pedro.rogerio Exp $
*/
#ifndef _AK_TYPES_H
#define _AK_TYPES_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*! Definicoes de plataforma */
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define AK_WIN32
#elif (defined(__linux__) || defined(__linux))
#  define AK_LINUX
#elif (defined(__FreeBSD__) || defined(__DragonFly__))
#  define AK_FREEBSD
#elif (defined(__APPLE__) || defined(__MACH__))
#  define AK_APPLE
#endif

/* Definicoes dos tipos nao variaveis entre plataformas */
/* definicao relativa ao tratamento de enderecos ipv6 */
#ifdef AK_WIN32
#  define s6_addr32 s6_words
#endif

typedef	char                    aki8;
typedef	unsigned char           aku8;
typedef	short                   aki16;
typedef	unsigned short          aku16;
typedef	int                     aki32;
typedef	unsigned int            aku32;

#ifdef FW_X86_64
typedef unsigned long akptr;
#else
typedef unsigned int akptr;
#endif

#if defined(AK_LINUX) || defined(AK_FREEBSD) || defined(AK_APPLE)

typedef	long long               aki64;
typedef	unsigned long long      aku64;

#define akusleep(P) usleep(P)

#elif defined(AK_WIN32)

typedef __int64                 aki64;
typedef unsigned __int64        aku64;

#endif

#if defined(FW_DOUBLE_PTR)
#  define FW_PTR(decl) decl; decl##_fw_double_ptr_dummy__
#elif defined(__GNUC__)
#  define FW_PTR(decl) decl __attribute__((packed)) __attribute__((aligned(4)))
#else
#  define FW_PTR(decl) decl
#endif

#if defined(AK_WIN32)
typedef unsigned __int64        u_int64_t;
typedef __int64                 int64_t;
typedef unsigned __int32        u_int32_t;
typedef __int32                 int32_t;
typedef unsigned __int16        u_int16_t;
typedef __int16                 int16_t;
typedef unsigned __int8         u_int8_t;
typedef __int8                  int8_t;
typedef unsigned char           u_char;
typedef unsigned short          u_short;
typedef unsigned int            u_int;
typedef unsigned long           u_long;
typedef int                     socklen_t;
typedef long                    off_t;
typedef long                    ssize_t;
#define akusleep(P)   Sleep( ((P)/1000) ? ((P)/1000) : 1 )
#else
# if !defined(KERNEL) && !defined(__KERNEL__)
#   include <sys/types.h>
# endif
#endif

//! tipo do ID da maquina (usando na identificacao de membros de um cluster)

typedef aku64 akid;

// Definicoes para trabalhar com inteiros de 64 bits

#ifndef ntohll
#define ntohll(x) (ntohl((aku32)((x) >> 32)) == (aku32)((x) >> 32)) ? (x) : \
                  (((aku64)(ntohl((aku32)(((x) << 32) >> 32))) << 32) | \
                  (aku32)ntohl(((aku32)((x) >> 32))))
#endif

#ifndef htonll
#define htonll(x) (htonl((aku32)((x) >> 32)) == (aku32)((x) >> 32)) ? (x) : \
                  (((aku64)(htonl((aku32)(((x) << 32) >> 32))) << 32) | \
                  (aku32)htonl(((aku32)((x) >> 32))))
#endif
#endif //_AK_TYPES_H
