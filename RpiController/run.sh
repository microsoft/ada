#!/bin/bash
while :
do
  if [ "`ping -c 1 ada-core`" ]
  then
    echo "network is available"
    break
  else
    echo "ping failed, waiting for network..."
    sleep 1
  fi
done

while true
do
    ~/git/Ada/RpiController/build/bin/RpiController --autostart --firmware ~/git/Ada/TeensyFirmware/firmware.hex
    sleep 10
    pushd ~/git/Ada/TeensyFirmware
    ./flash.sh
    popd
    sleep 10
done

