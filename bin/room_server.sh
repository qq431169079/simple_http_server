#!/bin/bash

export LD_LIBRARY_PATH="/usr/local/lib"
export PATH=/usr/local/bin:/bin:/usr/bin:/usr/local/sbin:/usr/sbin:/sbin

name=room_server
chmod 755 $name

start()
{
    pid=`pidof $name`
    if [ $pid ];
    then
        echo "service already running. pid=$pid"
        return
    fi
    nohup ./$name &>/dev/null &
    echo "$name start"
}

stop()
{
    pid=`pidof $name`
    if [ ! $pid ];
    then
        echo "service already exit"
        return
    fi

    kill -TERM `pidof $name`
    ret=$?
    if [ $ret -eq 0 ]
    then
        echo "$name stop"
    else
        echo "$name stop failed"
    fi
    return
}

monitor()
{
    pid=`pidof $name`
    if [ ! $pid ];
    then
        start
        return
    else
        echo "service already running. pid=$pid"
    fi
    return
}

restart()
{
    stop
    start
    return
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    monitor)
        monitor
        ;;
    *)
        echo $"Usage: $0 {start|stop|restart|monitor}"
        exit 1
esac

exit 0
