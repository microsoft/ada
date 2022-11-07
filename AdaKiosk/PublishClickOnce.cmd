cd %dp0
if "%ADA_STORAGE_CONNECTION_STRING%x" == "x" goto :noconnection
if not exist bin\publish goto :nopublish
call AzurePublishClickOnce.cmd bin\publish adakiosk/ClickOnce "%ADA_STORAGE_CONNECTION_STRING%"
if ERRORLEVEL 1 goto :eof

pwsh -f UploadZip.ps1

goto :eof

:nopublish
echo Please publish clickonce to bin\publish folder
goto :eof

:noconnection
echo Please set ADA_STORAGE_CONNECTION_STRING