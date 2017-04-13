#!/bin/sh

#frequency used
HDD_BANDWIDTH_PASS=3
IPERF_BANDWIDTH_PASS=50
SER_BANDWIDTH_PASS=10
LOGFILE_DIR=/tmp
#IPERF_BANDWIDTH_PASS=150

#iperf configuration
IPERF_SERVER_IP=192.168.88.10
IPERF_SERVER_PORT=9735
IPERF_LISTEN_PORT=9734
IPERF_TIME_IN_SEC=10
#MBIT/sec
#IPERF_BANDWIDTH=1000M
IPERF_BANDWIDTH=100M
#IPERF_BANDWIDTH=20M
IPERF_INTERVAL=0
IPERF_DEV=fec0
IPERF_OUTPUT_FILE=/tmp/${IPERF_DEV}_iperf.log

#HDD configuration
#MiB/sec
#HDD_BLOCK_SIZE=256
#HDD_NUM_OF_MEGS=256
HDD_BLOCK_SIZE=128
HDD_NUM_OF_MEGS=128
HDD0_OUTPUT_FILE=/tmp/hd0_zcav.log
HDD1_OUTPUT_FILE=/tmp/hd1_zcav.log
HDD2_OUTPUT_FILE=/tmp/hd2_zcav.log
HDD3_OUTPUT_FILE=/tmp/hd3_zcav.log

#Serial port loopback configuration
SER_DEV=ser5
SER_BAUNDRATE=115200
SER_TEXT="1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!~@#$%^&*()[]{}_+-=<>/?':;,."
SER_OUTPUT_FILE=/tmp/${SER_DEV}_loopback.log

debug_printf() {
	TIMESTAMP=`date`
	echo "$TIMESTAMP $1"
	echo "$TIMESTAMP $1" >> $2
}

check_storage_for_report() {
	if [ -d /mnt/hd0/dos ]; then
		LOGFILE_DIR=/mnt/hd0/dos
	elif [ -d /mnt/hd1/dos ]; then
		LOGFILE_DIR=/mnt/hd1/dos
	elif [ -d /mnt/hd2/dos ]; then
		LOGFILE_DIR=/mnt/hd2/dos
	elif [ -d /mnt/hd3/dos ]; then
		LOGFILE_DIR=/mnt/hd3/dos
	else
		LOGFILE_DIR=/tmp/
	fi

	IPERF_REPORT=$LOGFILE_DIR/${IPERF_DEV}_report.log
	HDD0_REPORT=$LOGFILE_DIR/hd0_report.log
	HDD1_REPORT=$LOGFILE_DIR/hd1_report.log
	HDD2_REPORT=$LOGFILE_DIR/hd2_report.log
	HDD3_REPORT=$LOGFILE_DIR/hd3_report.log
	SER_REPORT=$LOGFILE_DIR/${SER_DEV}_report.log
	MONITOR_REPORT=$LOGFILE_DIR/monitor_report.log
}

iperf_client_speed_test_with_time() {
	if [ $IPERF_INTERVAL -eq 0 ]; then
		iperf -c $IPERF_SERVER_IP -b $IPERF_BANDWIDTH -t $IPERF_TIME_IN_SEC -p $IPERF_SERVER_PORT -L $IPERF_LISTEN_PORT -r > $IPERF_OUTPUT_FILE
	else
		iperf -c $IPERF_SERVER_IP -i $IPERF_INTERVAL -b $IPERF_BANDWIDTH -t $IPERF_TIME_IN_SEC -p $IPERF_SERVER_PORT -L $IPERF_LISTEN_PORT -r > $IPERF_OUTPUT_FILE
	fi
}

iperf_client_speed_test_loop() {

	check_storage_for_report
	n=0
	NET_NAME=$1
	rm -f $IPERF_REPORT
	while [ $n -lt 100000 ]
	do
		n=$(( n+1 ))	 # increments $d
		rm -f $IPERF_OUTPUT_FILE
		iperf_client_speed_test_with_time
		RESULT=$?
		if [ $RESULT -eq 0 ]; then
			TIMEOUT=`cat $IPERF_OUTPUT_FILE | grep 'timed out'`
			if [ -z $TIMEOUT ]; then
				BANDWIDTH=`cat $IPERF_OUTPUT_FILE | grep 'bits/sec' | awk '{print $7}'`
				UNIT=`cat $IPERF_OUTPUT_FILE | grep 'bits/sec' | awk '{print $8}'`
				if [ -z $BANDWIDTH ]; then
					debug_printf "########## $NET_NAME, n=$n, can't find the bandwidth result#1, try it again ##########" $IPERF_REPORT
					#exit 1;
				elif [ "$BANDWIDTH" == "Bytes" -o "$BANDWIDTH" == "MBytes" ]; then
					debug_printf "########## $NET_NAME, n=$n, can't find the bandwidth result#2, try it again ##########" $IPERF_REPORT
					#exit 1;
				else
					debug_printf "[$NET_NAME, n=$n, $BANDWIDTH $UNIT]" $IPERF_REPORT
					BANDWIDTH=`echo ${BANDWIDTH%\.*}`
					if [ $BANDWIDTH -lt $IPERF_BANDWIDTH_PASS ]; then
						debug_printf "########## $NET_NAME, n=$n, bandwidth < $IPERF_BANDWIDTH_PASS $UNIT ##########" $IPERF_REPORT
						#exit 1;
					fi
				fi
			else	
				debug_printf "########## $NET_NAME, n=$n, connection timeout, try it again ##########" $IPERF_REPORT
			fi
		else
			debug_printf "########## $NET_NAME, n=$n, errno=$RESULT, try it again ##########" $IPERF_REPORT
			#exit 1;
		fi
		sleep 5
	done
}

iperf_server() {
	iperf -s -p $SERVER_PORT
}

hdd_speed_test() {
	HDD_DEV_NAME=$1
	OUTPUT_FILE=$2
	zcav -f /dev/$HDD_DEV_NAME -b $HDD_BLOCK_SIZE -n $HDD_NUM_OF_MEGS > $OUTPUT_FILE
}

hdd_speed_test_once() {
	n=$5
	DEV_NAME=$1
	OUTPUT_FILE=$2
	BANDWIDTH_PASS=$3
	HDD_REPORT=$4
	rm -f $OUTPUT_FILE
	hdd_speed_test $DEV_NAME $OUTPUT_FILE
	RESULT=$? 
	if [ $RESULT -eq 0 ]; then
		BANDWIDTH=`cat $OUTPUT_FILE | sed -sn 3p | awk '{print $2}'`
		UNIT=`cat $OUTPUT_FILE | sed -sn 2p | awk '{print $4}' | sed s/,//`
		if [ -z $BANDWIDTH ]; then
			debug_printf "########## $DEV_NAME, n=$n, can't find the bandwidth result, try it again ##########" $HDD_REPORT
			cat $OUTPUT_FILE
			#exit 1;
		else
			debug_printf "[$DEV_NAME, n=$n, $BANDWIDTH $UNIT]" $HDD_REPORT
			BANDWIDTH=`echo ${BANDWIDTH%\.*}`
			if [ $BANDWIDTH -lt $BANDWIDTH_PASS ]; then
				debug_printf "########## $DEV_NAME, n=$n, bandwidth < $BANDWIDTH_PASS $UNIT ##########" $HDD_REPORT
				cat $OUTPUT_FILE
				#exit 1;
			fi
		fi
	else
		debug_printf "########## $DEV_NAME, n=$n, errno=$RESULT, retry again ##########" $HDD_REPORT
		cat $OUTPUT_FILE
		#exit 1;
	fi

}

hdd_speed_test_loop() {
	check_storage_for_report
	DEV_NAME=$1
	OUTPUT_FILE=$2
	BANDWIDTH_PASS=$3
	HDD_REPORT=$4
	TOTAL_TIMES=$5
	COUNT=$6
	y=0
	if [ -e /dev/$DEV_NAME ]; then
		while [ $y -lt $TOTAL_TIMES ]
		do
			y=$(( y+1 ))	 # increments $d
			if [ $TOTAL_TIMES -eq 1 ]; then
				hdd_speed_test_once $DEV_NAME $OUTPUT_FILE $BANDWIDTH_PASS $HDD_REPORT $COUNT
			else
				hdd_speed_test_once $DEV_NAME $OUTPUT_FILE $BANDWIDTH_PASS $HDD_REPORT $y
			fi
		done
	fi
}

hdd_test() {
	check_storage_for_report
	# for sdcard/usb0/usb1
	RUN_AT_THE_SAME_TIME=$1
	rm -f $HDD0_REPORT
	rm -f $HDD1_REPORT
	rm -f $HDD2_REPORT
	rm -f $HDD3_REPORT

	if [ $RUN_AT_THE_SAME_TIME -eq 0 ]; then
		x=0
		while [ $x -lt 100000 ]
		do
			x=$(( x+1 ))	 # increments $d
			hdd_speed_test_loop hd0 $HDD0_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD0_REPORT 1 $x
			hdd_speed_test_loop hd1 $HDD1_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD1_REPORT 1 $x
			hdd_speed_test_loop hd2 $HDD2_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD2_REPORT 1 $x
			hdd_speed_test_loop hd3 $HDD3_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD3_REPORT 1 $x
		done

	else
		hdd_speed_test_loop hd0 $HDD0_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD0_REPORT 100000 &
		hdd_speed_test_loop hd1 $HDD1_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD1_REPORT 100000 &
		hdd_speed_test_loop hd2 $HDD2_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD2_REPORT 100000 &
		hdd_speed_test_loop hd3 $HDD3_OUTPUT_FILE $HDD_BANDWIDTH_PASS $HDD3_REPORT 100000 &
	fi
}

serial_loopback_test_once() {
	DEV_NAME=$1
	BAUDRATE=$2
	TEXT=$3
	rm -f $SER_OUTPUT_FILE
	serial-loopback-test /dev/$DEV_NAME $BAUDRATE $TEXT > $SER_OUTPUT_FILE
}

serial_loopback_test_loop() {
	check_storage_for_report
	n=0
	DEV_NAME=$1
	BAUDRATE=$2
	TEXT=$3
	rm -f $SER_REPORT
	while [ $n -lt 100000 ]
	do
		n=$(( n+1 ))	 # increments $d
		serial_loopback_test_once $DEV_NAME $BAUDRATE $TEXT
		RESULT=$?
		if [ $RESULT -eq 0 ]; then
			BANDWIDTH=`cat $SER_OUTPUT_FILE | grep 'Measured' | awk '{print $9}'`
			UNIT=`cat $SER_OUTPUT_FILE | grep 'Measured' | awk '{print $10}'`
			if [ -z $BANDWIDTH ]; then
				debug_printf "########## $DEV_NAME, n=$n, can't find the bandwidth result#1, try it again ##########" $SER_REPORT
				#exit 1;
			else
				debug_printf "[$DEV_NAME, n=$n, $BANDWIDTH $UNIT]" $SER_REPORT
				BANDWIDTH=`echo ${BANDWIDTH%\.*}`
				if [ $BANDWIDTH -lt $SER_BANDWIDTH_PASS ]; then
					debug_printf "########## $DEV_NAME, n=$n, bandwidth < $SER_BANDWIDTH_PASS $UNIT ##########" $SER_REPORT
					#exit 1;
				fi
			fi
		else
			debug_printf "########## $DEV_NAME, n=$n, errno=$RESULT, try it again ##########" $SER_REPORT
			#exit 1;
		fi
		sleep 5
	done
}

monitor_process() {
	n=0
	PRE_PID=0
	PROG_NAME=$1
	REPORT_FILE=$2
	while [ $n -lt 100000 ]
	do
		sleep 20
		TMP_CUR_PID=`pidin ar | grep $PROG_NAME | sed -sn 1p | awk '{print $2}'`
		if [ $TMP_CUR_PID == $PROG_NAME ]; then
			CUR_PID=`pidin ar | grep $PROG_NAME | sed -sn 1p | awk '{print $1}'`
			if [ ! -z $CUR_PID ]; then
				if [ $CUR_PID -eq $PRE_PID ]; then
					n=$(( n+1 ))
					debug_printf "########## monitor:$PROG_NAME, n=$n, slay $CUR_PID ##########" $REPORT_FILE
					slay $PROG_NAME
				else
					PRE_PID=$CUR_PID
				fi
			fi
		fi
	done
}

monitor_multi_process() {
	check_storage_for_report
	monitor_process serial-loopback-test $MONITOR_REPORT &
	monitor_process iperf $MONITOR_REPORT &
	monitor_process zcav $MONITOR_REPORT &
}

fullload_start() {

	# for sdcard/usb0/usb1
	hdd_test 0 &

	# for fec0
	iperf_client_speed_test_loop $IPERF_DEV &

	# for ser5
	serial_loopback_test_loop $SER_DEV $SER_BAUNDRATE $SER_TEXT &

	#for graphic
	vg-tiger > /dev/null &

	#for monitor process
	monitor_multi_process &
}


fullload_stop() {
	GET_FULLLOAD_PID=`pidin ar | grep fullload.sh | sed -sn 1p | awk '{print $1}'`
	while [ ! -z $GET_FULLLOAD_PID ]
	do
		FULLLOAD_PID=`pidin ar | grep fullload.sh | sed -sn 1p | awk '{print $1}'`
		echo slay fullload_pid:$FULLLOAD_PID
		slay $FULLLOAD_PID
		sleep 1
	done
	slay vg-tiger > /dev/null
}


case "$1" in
  start)
	IPERF_SERVER_IP=$2
	fullload_start $IPERF_SERVER_IP
        ;;
  stop)
	fullload_stop
	;;

  hdd)
	if [ -z $2 ]; then
		hdd_test 0
	else
		DEV_NAME=$2
		hdd_speed_test_loop $DEV_NAME $LOGFILE_DIR/${DEV_NAME}_zcav.log $HDD_BANDWIDTH_PASS $LOGFILE_DIR/${DEV_NAME}_report.log 99999
	fi
	;;
  iperf)
	IPERF_SERVER_IP=$2
	iperf_client_speed_test_loop $IPERF_DEV &
	;;
  serial)
	serial_loopback_test_loop $SER_DEV $SER_BAUNDRATE $SER_TEXT &
	;;
  monitor)
	monitor_multi_process &
	;;
  *)
	echo $"Usage: $0 {start iperf_server_ip|stop|hdd|iperf iperf_server_ip|serial}"
	exit 1
esac

exit $?
