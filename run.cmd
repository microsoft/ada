pushd %~dp0
call conda activate ada

pwsh -f Server\find_network.ps1

pushd Server
start pwsh -f run_server.ps1
popd
