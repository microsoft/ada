pushd %~dp0
call "C:\ProgramData\Anaconda3\Scripts\activate.bat" Ada

powershell -f Server\find_network.ps1

pushd Server
start powershell -f run_server.ps1
popd

start start powershell -f Server\run_client.ps1 -name adapi1
start start powershell -f Server\run_client.ps1 -name adapi2
start start powershell -f Server\run_client.ps1 -name adapi3

REM pushd IPCameraGUI
REM start cmd /K run_camera.cmd
REM popd

pushd DmxController
start powershell -f run_dmx.ps1
popd

pushd KasaBridge
start powershell -f run_bridge.ps1
popd
