#!/bin/bash
# Builds the Aker Client kernel module on the user's machine during the product installation
# Created by Clesio S. Moura on 26/12/2007
# $Id: buildkernelmod64.sh,v 1.1.2.1 2013/02/19 20:06:35 artur.soares Exp $

INSTALL_DIR=/usr/local/AkerClient
COMP_KERNEL_FILE="${INSTALL_DIR}/driver/last_compiled_kernel"
KERNEL_VERSION=`uname -a`
FEDORA=`awk '/Fedora/ { print $0 }' /etc/issue`
UBUNTU=`awk '/buntu/ { print $0 }' /etc/issue`
DEBIAN=`awk '/ebian/ { print $0 }' /etc/issue`
MINT=`awk '/Mint/ { print $0 }' /etc/issue`
OPENSUSE=`awk '/openSUSE/ { print $0 }' /etc/issue`
SLACK=`awk '/elcome to/ { print $0 }' /etc/issue`
GENTOO=`awk '/Base System/ { print $0 }' /etc/gentoo-release > /dev/null 2>&1`

echo "Building Aker Client kernel module..."

# Checa se o modulo ja esta compilado para a versao atual do kernel.
if [ "`cat ${COMP_KERNEL_FILE} 2>> /dev/null`" == "${KERNEL_VERSION}" ] && [ -e "${INSTALL_DIR}/aker_client.ko" ]
then
  echo
  echo "Aker Client module was already built."
  echo
  exit 0
fi

cd /usr/src

rm -f linux-2.6

if [ -n "${FEDORA}" ]
then
  RET=`uname -r | grep fc8`
  if [ "${RET}" != "" ]
  then
    ln -sf kernels/`uname -r`-`uname -m` linux-2.6
  else
    ln -sf kernels/`uname -r` linux-2.6
  fi
elif [ -n "${UBUNTU}" ] || [ -n "${DEBIAN}" ] || [ -n "${MINT}" ]
then
  ln -sf linux-headers-`uname -r` linux-2.6
elif [ -n "${OPENSUSE}" ]
then
  RET=`uname -r | cut -d '-' -f 1`
  RET2=`uname -r | cut -d '-' -f 2`
  RET3=`uname -r | cut -d '-' -f 3`
  ln -sf linux-$RET-$RET2-obj/x86_64/$RET3 linux-2.6
elif [ -n "${GENTOO}" ]
then
  ln -sf linux-`uname -r` linux-2.6
elif [ -n "${SLACK}" ]
then
  RET=`uname -r | cut -d '-' -f 1`
  ln -sf linux-$RET linux-2.6
fi

cd ${INSTALL_DIR}/driver
RET=`make STAND_ALONE=1 > /dev/null`
echo $RET


if [ "X${RET}" != "X" ] && [ "X${RET}" != "X0" ]
then
  make clean > /dev/null
  echo
  echo "Failed to build Aker Client module!"
  ls -l /usr/src/linux-2.6
  echo
  exit 1
else
  cp -f aker_client.ko ${INSTALL_DIR}
  make clean > /dev/null
  uname -a > ${COMP_KERNEL_FILE}
  echo
  echo "Aker Client module was successfully built."
  echo
fi

rm -f linux-2.6

exit 0
