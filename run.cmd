pushd %~dp0
call conda activate ada

powershell -f Server\find_network.ps1

pushd Server
start powershell -f run_server.ps1
popd
