#!/usr/bin/env bash

ROOT_DIR=.
BASE_DIR=$ROOT_DIR
DATA_DIR=$(dirname $0)

PID_FILE=$BASE_DIR/qemu_bg.pid

CONT_BASE=$ROOT_DIR/scripts/continuous
CONT_RUN=$CONT_BASE/run.sh

EXPECT_TESTS_BASE=$ROOT_DIR/scripts/expect

EMBOX_IP=10.0.2.16
HOST_IP=10.0.2.10

HTTP_GOLD_SIMPLE_FILE="http_gold_simple_file.html"

export PEER_HOST_IP=192.168.128.128 # also hardcoded into x86/test/net start_script

TEST_PING_FORWARING_SCRIPT=$CONT_BASE/net/forwarding/test_ping_forwarding.sh

test_case_target_should_reply_to_ping() {
	ping $EMBOX_IP -c 4
	test_retcode
}

test_case_target_should_reply_to_big_ping() {
	ping $EMBOX_IP -c 4 -s 16384
	test_retcode
}

test_case_correct_index_html_should_be_downloaded() {

	wget $EMBOX_IP/$HTTP_GOLD_SIMPLE_FILE
	test_retcode

	diff -q $HTTP_GOLD_SIMPLE_FILE $DATA_DIR/$HTTP_GOLD_SIMPLE_FILE
	test_retcode

	rm $HTTP_GOLD_SIMPLE_FILE
}

#test_case_ssh_should_be_able_to_execute_command_and_show_output() {
#}

test_case_snmp_should_reply() {
	str=$(snmpget -v 1 -c public $EMBOX_IP 1.3.6.1.2.1.2.2.1.6.2)
	test_retcode

	echo $str | grep "AA BB CC DD EE 02" >/dev/null
	test_retcode
}

test_case_interactive_tests_should_success() {
	sudo killall in.rlogind

	expect $EXPECT_TESTS_BASE/framework/run_all.exp \
		$EXPECT_TESTS_BASE/x86_net_tests.config 10.0.2.16 10.0.2.10 ""

	test_retcode

	# This file contains thw full log, which can be relatively large, so we do not
	# print it to standard output.
	#
	# cat testrun.log
}

#test_case_ftp_should_be_able_to_upload_a_file() {
#}

test_suite_code=0

test_code=0
test_desc=""
test_begin() {
	test_desc=$1
	test_code=0
}

test_retcode() {
	if [ 0 -ne $? ]; then
		test_suite_code=1
		test_code=1

		echo "Test code is not 0"
	fi
}

test_end() {

	if [ 0 -ne $test_code ]; then
		echo Test case \"$test_desc\" failed
	else
		echo Test case \"$test_desc\" OK
	fi
}

tap_up() {
	sudo /sbin/ip tuntap add mode tap tap0
	sudo /sbin/ifconfig tap0 10.0.2.10 dstaddr 10.0.2.0 netmask 255.255.255.0 down
	sudo /sbin/ifconfig tap0 hw ether aa:bb:cc:dd:ee:ff up
	sudo /sbin/ifconfig tap0 inet6 del fe80::a8bb:ccff:fedd:eeff/64
	sudo /sbin/ifconfig tap0 inet6 add fe80::10:0:2:10/64

	local gw=$(/sbin/ip route | sed -n "s/default via .* dev \([0-9a-z_]\+\) .*$/\1/p")
	echo gw=$gw
	if [ "$gw" ]; then
		echo "Enable IP Forwarding for $gw"
		sudo iptables -t nat -A POSTROUTING -o $gw -j MASQUERADE
		sudo sysctl net.ipv4.ip_forward=1
	fi

	sudo service isc-dhcp-server start

	PTTAP=tap78
	sudo ip tuntap add dev $PTTAP mode tap
	sudo ip link set   dev $PTTAP address aa:bb:cc:dd:ef:01
	sudo ip link set   dev $PTTAP up
	sudo ip addr flush dev $PTTAP
	sudo ip addr add   dev $PTTAP 192.168.128.1/24
	sudo ip addr add   dev $PTTAP fe80::192:168:128:1/64

	export EMBOX_USERMODE_TAP_NAME=$PTTAP
	./ping-target &
}

tap_down() {
	pkill ping-target
	sudo /sbin/ip tuntap del mode tap $PTTAP
	sudo service isc-dhcp-server stop
	sudo /sbin/ip tuntap del mode tap tap0
}

sudo /etc/init.d/ntp restart
sudo inetd

cp $CONT_BASE/net/$HTTP_GOLD_SIMPLE_FILE $ROOT_DIR/conf/rootfs/$HTTP_GOLD_SIMPLE_FILE
make >/dev/null 2>/dev/null

tap_up

export AUTOQEMU_NICS_CONFIG="tap,ifname=tap0,script=no,downscript=no,vnet_hdr=no"
export CONTINIOUS_RUN_TIMEOUT=60
$CONT_RUN generic/qemu_bg "" $PID_FILE
if [ 0 -ne $? ]; then
	run_failed=1
fi

if [ ! $run_failed ]; then
	case_prefix=test_case_
	for test in $(declare -F | cut -d \  -f 3 | grep "^$case_prefix"); do
		test_begin "$(echo ${test#$case_prefix} | tr _ \ )"
		${test}
		test_end
	done

	$CONT_RUN generic/qemu_bg_kill "" $PID_FILE
	rm $PID_FILE
else
	test_suite_code=1

fi

tap_down

test_begin "ping forwarding test suite"
	$TEST_PING_FORWARING_SCRIPT
	test_retcode
test_end

exit $test_suite_code
