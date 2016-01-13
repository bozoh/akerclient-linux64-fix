#!/bin/sh

OPENSUSE=`awk '/openSUSE/ { print $0 }' /etc/issue`

if [ -n "${OPENSUSE}" ]
then
  xhost +local:${USER}
fi

USER_ID=`id -u`
AC_PID=`ps -u ${USER_ID} u | awk '/AkerClient\/akerclient/ && !/awk/ && !/akerclient_init.sh/ {print $2}'`

if [ -n "${AC_PID}" ]
then
  kill -s HUP ${AC_PID}
  exit
fi

env LD_LIBRARY_PATH=/usr/local/AkerClient/libs:$LD_LIBRARY_PATH \
/usr/local/AkerClient/akerclient &
