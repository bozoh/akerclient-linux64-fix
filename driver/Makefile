# Makefile para o driver do Aker Client - Versao Linux
# 05/09/2007 Criacao 				Rodrigo Ormonde
#
# $Id: Makefile,v 1.4 2010/10/20 16:52:37 neuton.martins Exp $

AK_CLIENT_FILES=ak_client_drv.o ak_client_hook.o md5.o

########## KBUILD part of Makefile, invoked by the Kbuild system in kernel 2.6.x ##########
ifneq ($(KERNELRELEASE),)

ifdef STAND_ALONE
 EXTRA_CFLAGS += -I${M} -DSTAND_ALONE
else
 DCOMMON=${M}/../../../common
 DTOOLS=${M}/../../../tools
 EXTRA_CFLAGS += -I${M} -I${DCOMMON}/fw_sr -I${DCOMMON}/fw-include/firewall \
		-I${DCOMMON}/fw-include/itexto \
		-I${DCOMMON}/fw-include/kernel -I${DTOOLS}/utils \
		-I${DTOOLS}/auth_api -I${DTOOLS}/include -I${DTOOLS}/url_api
endif

obj-m  := aker_client.o
aker_client-y := ${AK_CLIENT_FILES}

############################### NORMAL Makefile ############################
else

KERNELDIR=/usr/src/linux-2.6
DCOMMON=../../../common
DTOOLS=../../../tools

ifdef STAND_ALONE
 CFLAGS +=	-I. -DSTAND_ALONE
else
 CFLAGS +=	-I. -I${DCOMMON}/fw_sr -I${DCOMMON}/fw-include/firewall	\
		-I${DCOMMON}/fw-include/itexto \
		-I${DCOMMON}/fw-include/kernel -I${DTOOLS}/utils	\
		-I${DTOOLS}/auth_api -I${DTOOLS}/include -I${DTOOLS}/url_api
endif

all:
			$(MAKE) -C ${KERNELDIR} M=`pwd` KBUILD_VERBOSE=0
clean:
			$(MAKE) -C ${KERNELDIR} M=`pwd` clean
			@rm -f *.o core *.ko
endif #kbuild
