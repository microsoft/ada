#!/bin/bash

cd ~/git/Ada/TeensyFirmware
# using teensy_loader from https://github.com/PaulStoffregen/teensy_loader_cli
/home/pi/git/teensy_loader_cli/teensy_loader_cli --mcu=imxrt1062 -w -v firmware.hex &
sleep 1
gpio mode 7 out;gpio write 7 0; gpio mode 7 in
sleep 1

