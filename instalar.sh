#!/bin/bash

___OS___=`uname -s`
OS_VERSION=`uname -r`

OS_COMP=`echo $OS_VERSION | cut -d '-' -f 2`
OS_VERSION=`echo $OS_VERSION | cut -d '-' -f 1`

MAJOR=`echo $OS_VERSION | cut -d '.' -f 1`
MINOR=`echo $OS_VERSION | cut -d '.' -f 2`
RELEASE=`echo $OS_VERSION | cut -d '.' -f 3`
BUILD=`echo $OS_VERSION | cut -d '.' -f 4`

#echo $___OS___
#echo $OS_VERSION
#echo $OS_COMP
#echo $MAJOR
#echo $MINOR
#echo $RELEASE
#echo $BUILD

FEDORA=`awk '/Fedora/ { print $0 }' /etc/issue`
UBUNTU=`awk '/buntu/ { print $0 }' /etc/issue`
DEBIAN=`awk '/ebian/ { print $0 }' /etc/issue`
MINT=`awk '/Mint/ { print $0 }' /etc/issue`
OPENSUSE=`awk '/openSUSE/ { print $0 }' /etc/issue`
SLACK=`awk '/elcome to/ { print $0 }' /etc/issue`
GENTOO=`awk '/Base System/ { print $0 }' /etc/gentoo-release > /dev/null 2>&1`

clear
echo
echo
echo "Aker Client - Script de instalacao"
echo "----------------------------------"
echo
echo "Copyright (c) 2012 Aker Security Solutions."
echo "Todos os direitos reservados."

ID=`id -u`
if [ $ID -ne 0 ]
then
  echo
  echo "ERRO: Este script so' pode ser executado pelo root."
  echo
  exit
fi

if [ "Linux" != ${___OS___} ]
then
  echo
  echo "ERRO: Este script pode ser executado apenas em plataformas GNU Linux."
  echo
  exit
fi

if [ ${MAJOR} -ne 2 ] && [ ${MAJOR} -ne 3 ] && [ ${MAJOR} -ne 4 ]
then
  echo
  echo "ERRO: As versoes do kernel suportadas são 2.6.x e 3.x.x ."
  echo "      Versao encontrada: ${OS_VERSION}"
  echo
  exit
fi

if [ ${MAJOR} -eq 2 ] && [ ${MINOR} -lt 6 ]
then
  echo
  echo "ERRO: Versoes de kernel inferior a 2.6 nao sao suportadas."
  echo "      Versao encontrada: ${OS_VERSION}"
  echo
  exit
fi

echo -n "Verificando dependencias para compilacao do modulo de kernel... "

  RET=`gcc --version | grep Copyright`
  if [ "${RET}" = "" ]
  then
    echo "ERRO."
    echo "Compilador GCC nao encontrado. Impossivel prosseguir."
    echo
    if [ -n "${FEDORA}" ]
    then
      echo "No Fedora instale o seguintes pacote:"
      echo "* gcc"
    elif [ -n "${UBUNTU}" ] || [ -n "${DEBIAN}" ] || [ -n "${MINT}" ]
    then
      echo "No Ubuntu e derivados instale o seguinte pacote:"
      echo "* build-essencial"
    fi
  echo
  echo -n "Deseja realmente prosseguir? (S/N - N para cancelar) "
  read -n 1 A
    echo
    exit
  fi

  RET=`make --version | grep Make`
  if [ "${RET}" = "" ]
  then
    echo "ERRO."
    echo "Ferramenta 'Make' nao encontrada. Impossivel prosseguir."
    echo
    exit
  fi

  if [ -n "${FEDORA}" ]
  then
    RET=`rpm -qa | grep kernel | grep devel`
    if [ -z "${RET}" ]
    then
      echo "ERRO."
      echo "Pacote kernel-devel nao esta' instalado no sistema. Impossivel prosseguir."
      echo
      exit
    fi
  elif [ -n "${UBUNTU}" ] || [ -n "${DEBIAN}" ] || [ -n "${MINT}" ]
  then
    if [ ! -d /usr/src/linux-headers-`uname -r` ]
    then
      echo "ERRO."
      echo "Os headers do kernel nao foram instalados no sistema ou nao batem com o kernel atual. Impossivel processeguir"
      echo "Instale e/ou atualize os seguintes pacotes:"
      echo " * linux-image-generic"
      echo " * linux-image-headers"
      echo
      exit
    fi
  elif [ -n "${OPENSUSE}" ]
  then
    RET=`uname -r | cut -d '-' -f 1`
    RET2=`uname -r | cut -d '-' -f 2`
    if [ ! -d /usr/src/linux-$RET-$RET2 ]
    then
      echo "ERRO"
      echo "Os headers do kernel nao foram instalados no sistema ou nao batem com o kernel atual. Impossivel processeguir"
      echo
      exit
    fi
  elif [ -n "${GENTOO}" ]
  then
    RET=`uname -r`
    if [ ! -d /usr/src/linux-$RET ]
    then
      echo "ERROR."
      echo "Os headers do kernel nao foram instalados no sistema ou nao batem com o kernel atual. Impossivel processeguir"
      echo
      exit
    fi
  elif [ -n "${SLACK}" ]
  then
    RET=`uname -r | cut -d '-' -f 1`
    if [ ! -d /usr/src/linux-$RET ]
    then
      echo "ERROR."
      echo "Os headers do kernel nao foram instalados no sistema ou nao batem com o kernel atual. Impossivel processeguir"
      echo
      exit
    fi
  fi
  echo OK.

echo
echo
echo "Iniciando instalacao do Aker Client... "

INSTALL_DIR=/usr/local/AkerClient
DRIVER_DIR=${INSTALL_DIR}/driver
INIT_DIR=/etc/init.d
SBIN_DIR=/sbin
RC_DIR=/etc
SHARE_DIR=/usr/share

./remover.sh >/dev/null 2>&1

mkdir -p ${INSTALL_DIR}/driver

install -m 755 bin/acservice ${INSTALL_DIR}
install -m 751 scripts/acservice.init ${INSTALL_DIR}
install -m 751 scripts/acservice_env ${INSTALL_DIR}
install -m 751 scripts/acservice_gentoo ${INSTALL_DIR}
install -m 755 bin/akerclient ${INSTALL_DIR}
install -m 755 scripts/akerclient_init.sh ${INSTALL_DIR}
install -m 644 bin/akerclientPT.qm ${INSTALL_DIR}
install -m 444 driver/ak_client_drv.c ${DRIVER_DIR}
install -m 444 driver/ak_client_drv.h ${DRIVER_DIR}
install -m 444 driver/ak_client_hook.c ${DRIVER_DIR}
install -m 444 driver/ak_stand_alone.h ${DRIVER_DIR}
install -m 444 driver/aktypes.h ${DRIVER_DIR}
install -m 444 driver/Makefile ${DRIVER_DIR}
install -m 444 driver/md5.c ${DRIVER_DIR}
install -m 444 driver/md5.h ${DRIVER_DIR}
install -m 744 driver/buildkernelmod.sh ${DRIVER_DIR}
install -m 755 bin/pam_aker_client.so ${INSTALL_DIR}
install -m 744 scripts/pam_ac_install.sh ${INSTALL_DIR}
install -m 744 scripts/pam_ac_install_ubuntu.sh ${INSTALL_DIR}

install -m 644 icons/akerclient.png ${SHARE_DIR}/pixmaps
install -m 644 icons/akerclient.desktop ${SHARE_DIR}/applications
install -m 644 icons/akerclient.desktop ${SHARE_DIR}/autostart
install -m 644 icons/akerclient.desktop ${SHARE_DIR}/gnome/autostart

install -m 751 remover.sh ${INSTALL_DIR}

cp -rf libs ${INSTALL_DIR}
ln -sf libXfixes.so.3 ${INSTALL_DIR}/libs/libXfixes.so

chcon -t textrel_shlib_t ${INSTALL_DIR}/libs/* >/dev/null 2>&1
chcon -t bin_t ${INSTALL_DIR}/pam_aker_client.so >/dev/null 2>&1
chcon -t textrel_shlib_t ${INSTALL_DIR}/pam_aker_client.so >/dev/null 2>&1

#Instala o servico
if [ -n "${OPENSUSE}" ]
then
  install -m 751 scripts/acservice.sh ${INIT_DIR}/acservice
  insserv ${INIT_DIR}/acservice >> /dev/null 2>> /dev/null
  ln -s ${INIT_DIR}/acservice ${SBIN_DIR}/rcacservice
elif [ -n "${GENTOO}" ]
then
  install -m 751 scripts/acservice_gentoo ${INIT_DIR}/acservice
  rc-update add acservice default
else
  install -m 751 scripts/acservice.sh ${INIT_DIR}/acservice
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc0.d/K14acservice.sh
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc1.d/K14acservice.sh
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc2.d/S14acservice.sh
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc3.d/S14acservice.sh
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc4.d/S14acservice.sh
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc5.d/S14acservice.sh
  ln -sf ${INIT_DIR}/acservice ${RC_DIR}/rc6.d/K14acservice.sh

  #Fedora com systemd tem que incluir o servico atraves do chkconf
  if [ -e /etc/systemd ] && [ -n "${FEDORA}" ]
  then
    chkconfig --add acservice
  fi
fi

if [ ! -d /dev/net ]
then
  mkdir /dev/net
fi

if [ ! -e /dev/net/tun ]
then
  mknod /dev/net/tun c 10 200
  chmod 0700 /dev/net/tun
fi

rm -f /dev/acservice
mknod /dev/acservice c 122 0
chmod 644 /dev/acservice

  echo
  echo -n "Instalando modulo de login transparente... "

  if [ -n "${UBUNTU}" ] || [ -n "${OPENSUSE}" ] || [ -n "${MINT}" ]
  then
    ${INSTALL_DIR}/pam_ac_install_ubuntu.sh
  else
    ${INSTALL_DIR}/pam_ac_install.sh  
  fi
  echo "OK."

  echo
  echo -n "Instalacao do modulo de kernel do Aker Client... "
  ${DRIVER_DIR}/buildkernelmod.sh
  if [ $? = 0 ]
  then
    echo "OK."
  else
    echo "Resolva as pendencias e execute ${DRIVER_DIR}/buildkernelmod.sh"
    echo "para compilar o modulo."
  fi

echo
echo -n "Iniciando o servico... "
${INIT_DIR}/acservice start
echo "OK."
touch /usr/local/AkerClient/configuration.xml
chmod o+w /usr/local/AkerClient/configuration.xml

echo
echo "Execute ${INSTALL_DIR}/akerclient_init.sh para iniciar a GUI do Aker Client."
