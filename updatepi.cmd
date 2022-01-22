@echo off
if %1=="" goto :noarg

pushd %~dp0
call "C:\ProgramData\Anaconda3\Scripts\activate.bat" Ada

scp -r c:\git\Ada\RpiController pi@%1:/home/pi/git/Ada

ssh pi@%1 -t chmod u+x /home/pi/git/Ada/RpiController/build.sh
ssh pi@%1 -t chmod u+x /home/pi/git/Ada/RpiController/run.sh
ssh pi@%1 -t /home/pi/git/Ada/RpiController/build.sh

goto :eof

:noarg
echo Please specify name of pi to update.