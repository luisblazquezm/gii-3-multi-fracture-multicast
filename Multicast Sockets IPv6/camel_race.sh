#!/bin/bash

GROUP_ADDRESS='ff15::33'
INTERFACE='eth0'
PORT='50000'
HOPS=15
DELAY=1
MESSAGE='AllOfYourBasesAreBelongToUs'

./difusor $MESSAGE $GROUP_ADDRESS $INTERFACE $PORT $DELAY $HOPS &

./suscriptor 1 $GROUP_ADDRESS $INTERFACE $PORT &
./suscriptor 2 $GROUP_ADDRESS $INTERFACE $PORT &
./suscriptor 3 $GROUP_ADDRESS $INTERFACE $PORT &


exit 0
