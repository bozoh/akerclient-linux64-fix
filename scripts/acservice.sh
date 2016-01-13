#!/bin/sh
# Inicializa/finaliza o servico do Aker Client

# acservice          Inicializa/finaliza o servico do Aker Client
#
# chkconfig: 2345 14 14
# description: Aker Client Service, provide authentication and \
#              VPN solution for Aker Firewall.
#
# processname: acservice

## BEGIN INIT INFO
# Provides:               acservice
# Required-Start:         $local_fs $network
# Required-Stop:          $local_fs
# Default-Start:          2 3 4 5
# Default-Stop:           0 1 6
# Short-Description:      Aker Client Service.
# Descripton:             Aker Client Service, provide authentication and
#                         VPN solution for Aker Firewall.
### END INIT INFO

CWD=`pwd`
DPATH=/usr/local/AkerClient/acservice_env
MPATH=/usr/local/AkerClient/aker_client.ko
PID_PATH=/var/run/acservice.pid
KERNEL_SCRIPT=/usr/local/AkerClient/driver/buildkernelmod.sh
KERNEL_LOG=/usr/local/AkerClient/driver/buildkernelmod.log

start()
{
  $KERNEL_SCRIPT >> $KERNEL_LOG 1>> $KERNEL_LOG 2>> $KERNEL_LOG
  /sbin/modprobe tun 2>> /dev/null
  /sbin/insmod $MPATH
  echo -n "Iniciando o servico do Aker Client: "
  $DPATH
  RETVAL=$?
  echo ""
  return $RETVAL  
}

stop()
{
  PID=`ps aux | awk '/AkerClient\/acservice/ && !/awk/ {print $2}'`
  echo -n "Finalizando o servico do Aker Client."
  if [ -n "${PID}" ]
  then
    kill -9 ${PID}
  fi
  /sbin/rmmod aker_client 2>> /dev/null
  echo ""
  return $RETVAL
}

status()
{
  RUNNING=`pidof acservice`
  if [ -n "${RUNNING}" ]
  then
    echo "Servico do Aker Client rodando."
    return 0
  fi

  echo "O servico do Aker Client nao esta rodando."
  return 1
}

case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		start
		;;
        status)
                status
                ;;
        force-reload)
                stop
                start
                ;;
        reload)
                stop
                start
                ;;
	*)
		echo ""
		echo "Uso: acservice.sh [ start | stop | restart | status ]"
		echo ""
		exit 1
		;;
esac

exit $RETVAL
