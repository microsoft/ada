pushd %~dp0
call "C:\ProgramData\Anaconda3\Scripts\activate.bat" Ada

start ssh pi@adapi1 -t /home/pi/git/Ada/RpiController/run.sh
start ssh pi@adapi2 -t /home/pi/git/Ada/RpiController/run.sh
start ssh pi@adapi3 -t /home/pi/git/Ada/RpiController/run.sh

REM pushd IPCameraGUI
REM start cmd /K run_camera.cmd
REM popd

pushd DmxController
start powershell -f run_dmx.ps1
popd

pushd Server
start powershell -f run_server.ps1
popd
