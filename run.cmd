pushd %~dp0
call "C:\ProgramData\Anaconda3\Scripts\activate.bat" Ada

powershell -f Server\find_network.ps1

pushd Server
start powershell -f run_server.ps1
popd
