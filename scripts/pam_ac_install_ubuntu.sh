#!/bin/sh
#
# Setup script for pam_aker_client.so module. Enables Aker Client transparent login on ubuntu.
#
# Created by Neuton M. Costa on 14/06/2010
#
# $Id: pam_ac_install_ubuntu.sh,v 1.1 2010/07/01 10:47:33 neuton.martins Exp $

if [ ! -d /etc/pam.d ]
then
  echo "PAM is not available. Aker Client transparent login will not be available."
  exit
fi

cd /etc/pam.d

#Desinstalacao (sh pam_ac_install_ubuntu.sh remove)
if [ "$1" = "remove" ]
then
  #Certifica que o arquivo original ainda existe
  if [ -f common-auth-bkp ]
  then
    #Copia por cima do arquivo alterado
    cp common-auth-bkp common-auth
    #Remove o arquivo de backup criado por esse script
    rm -f common-auth-bkp
  fi
  if [ -f common-session-bkp ]
  then
    #Copia por cima do arquivo alterado
    cp common-session-bkp common-session
    #Remove o arquivo de backup criado por esse script
    rm -f common-session-bkp
  fi
  #Encerra
  exit
fi

if [ ! -f common-auth ]
then
  echo "File /etc/pam.d/common-auth does not exist. Aker Client transparent login will not be available."
  exit
fi

if [ ! -f common-session ]
then
  echo "File /etc/pam.d/common-session does not exist. Aker Client transparent login will not be available."
  exit
fi

#Remove o arquivo que ocasionalmente ja pode existir (apenas append '>>' eh usado abaixo)
rm -f common-auth-temp
rm -f common-session-temp

while read LINHA
do
  #Grava no arquivo de destino com append
  echo $LINHA >> common-auth-temp
done < common-auth

#Inclui no final do arquivo common-auth
echo "auth sufficient /usr/local/AkerClient/pam_aker_client.so use_first_pass" >> common-auth-temp

while read LINHA
do
  #Grava no arquivo de destino com append
  echo $LINHA >> common-session-temp
done < common-session

#Inclui no final do arquivo common-session
echo "session optional /usr/local/AkerClient/pam_aker_client.so" >> common-session-temp

#Faz backup dos arquivos
cp common-auth common-auth-bkp
cp common-session common-session-bkp

#Copia por cima os arquivos alterados
cp common-auth-temp common-auth
cp common-session-temp common-session

#Remove os arquivos temporarios
rm -f common-auth-temp
rm -f common-session-temp
