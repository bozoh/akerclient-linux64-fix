#!/bin/bash

clear
echo
echo
echo "Aker Client - Script de desinstalacao"
echo "-------------------------------------"
echo
echo "Copyright (C) 2012 Aker Security Solutions. Todos os direitos reservados."
echo

ID=`id -u`
if [ $ID -ne 0 ]
then
  echo
  echo "ERRO: Este script so' pode ser executado pelo root."
  echo
  exit
fi

INSTALL_DIR=/usr/local/AkerClient
SHARE_DIR=/usr/share
INIT_DIR=/etc/init.d
RC_DIR=/etc
SBIN_DIR=/sbin

${INIT_DIR}/acservice stop
killall akerclient

UBUNTU=`awk '/buntu/ { print $0 }' /etc/issue`
OPENSUSE=`awk '/openSUSE/ { print $0 }' /etc/issue`
FEDORA=`awk '/Fedora/ { print $0 }' /etc/issue`
GENTOO=`awk '/Base System/ { print $0 }' /etc/gentoo-release  > /dev/null 2>&1`

  if [ -n "${UBUNTU}" ] || [ -n "${OPENSUSE}" ]
  then
    ${INSTALL_DIR}/pam_ac_install_ubuntu.sh remove
  else
    ${INSTALL_DIR}/pam_ac_install.sh remove
  fi

echo
echo -n "Removendo arquivos... "
if [ -f /usr/local/AkerClient/configuration.xml ]
then
  cp -f /usr/local/AkerClient/configuration.xml /tmp/configuration.xml
fi

#Removendo servico
if [ -n "${OPENSUSE}" ]
then
  insserv -r ${INIT_DIR}/acservice >> /dev/null 2>> /dev/null
  rm -f ${SBIN_DIR}/rcacservice
elif [ -n "${GENTOO}" ]
then
  rc-update del acservice default
else
  if [ -e /etc/systemd ] && [ -n "${FEDORA}" ]
  then
    chkconfig --del acservice
  fi

  for i in 0 1 2 3 4 5 6
  do
    rm -f ${RC_DIR}/rc$i.d/*acservice.sh
  done
fi

rm -rf ${INSTALL_DIR}
rm -f ${SHARE_DIR}/pixmaps/akerclient.png
rm -f ${SHARE_DIR}/applications/akerclient.desktop
rm -f ${SHARE_DIR}/autostart/akerclient.desktop
rm -f ${SHARE_DIR}/gnome/autostart/akerclient.desktop
rm -f ${INIT_DIR}/acservice

rm -f /dev/acservice

if [ -f /tmp/configuration.xml ]
then
  mkdir -p ${INSTALL_DIR}
  cp -f /tmp/configuration.xml ${INSTALL_DIR}/configuration.xml
  rm -f /tm/configuration.xml
fi

echo "OK."
