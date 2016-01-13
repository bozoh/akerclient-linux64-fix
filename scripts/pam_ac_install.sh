#!/bin/sh
#
# Setup script for pam_aker_client.so module. Enables Aker Client transparent login.
#
# Created by Clesio S. Moura on 11/10/2007
#
# $Id: pam_ac_install.sh,v 1.2 2010/03/22 12:27:02 hugo.saldanha Exp $

if [ ! -d /etc/pam.d ]
then
  echo "PAM is not available. Aker Client transparent login will not be available."
  exit
fi

cd /etc/pam.d

#Desinstalacao (sh pam_ac_install.sh remove)
if [ "$1" = "remove" ]
then
  #Certifica que o arquivo original ainda existe
  if [ -f system-auth-ac ]
  then
    #Volta o link simbolico para o valor original
    ln -sf system-auth-ac system-auth
    #Remove o arquivo criado por esse script
    rm -f system-auth-ac.aker
  fi
  #Encerra
  exit
fi

if [ ! -f system-auth-ac ]
then
  echo "File /etc/pam.d/system-auth-ac does not exist. Aker Client transparent login will not be available."
  exit
fi

#Remove espacos em branco redundantes de system-auth-ac e grava em system-auth-ac.pmt
tr -s " " < system-auth-ac > system-auth-ac.pmt

#Muda o controle de sufficient para required e escreve em system-auth-ac.tmp
sed 's/auth sufficient pam_unix.so/auth required pam_unix.so/' < system-auth-ac.pmt > system-auth-ac.tmp

#Esse arquivo nao em mais necessario (tmp do tmp)
rm -f system-auth-ac.pmt

#Remove o arquivo que ocasionalmente ja pode existir (apenas append '>>' eh usado abaixo)
rm -f system-auth-ac.aker

MODINSTALLED=0

while read LINHA
do
  #Grava no arquivo de destino, system-auth-ac.aker, com append
  echo $LINHA >> system-auth-ac.aker

  #Testa se o modulo pam_aker_client.so ja foi instalado
  if [ $MODINSTALLED = 0 ]
  then
    FOUNDGROUP=0
    for PALAVRA in $LINHA
    do
      #Verifica se eh do grupo de autenticacao
      if [ "X$PALAVRA" = "Xauth" ]
      then
        FOUNDGROUP=1
        continue
      fi

      if [ "X$PALAVRA" = "Xpam_unix.so" -a $FOUNDGROUP = 1 ]
      then
        echo "auth sufficient /usr/local/AkerClient/pam_aker_client.so use_first_pass" >> system-auth-ac.aker
        MODINSTALLED=1
        break
      fi
    done
  fi

done < system-auth-ac.tmp

rm -f system-auth-ac.tmp

#Inclui no final do arquivo a entrada para o grupo session
echo "session optional /usr/local/AkerClient/pam_aker_client.so" >> system-auth-ac.aker

#Muda o alvo do link simbolico (Alvo original: system-auth -> system-auth-ac)
ln -sf system-auth-ac.aker system-auth
