/* fwmd5.h - Arquivo header para o modulo fwmd5.c

  E' necessario incluir sys/param.h, sys/systm.h e sys/mbuf.h  */

#ifndef _MD5_H
#define _MD5_H

typedef unsigned char *POINTER;
typedef unsigned short int UINT2;
typedef unsigned int UINT4;

typedef struct
{
  UINT4 state[4];                                   /* state (ABCD) */
  UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
  unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);

#endif
