@echo off
cd %~dp0

if not EXIST bin\publish goto :nobits
if "%ADA_KIOSK_STORAGE_CONNECTION_STRING%" == "" goto :nokey

echo ### Uploading ClickOnce installer to Azure
AzurePublishClickOnce %~dp0bin\publish downloads/AdaKiosk "%ADA_KIOSK_STORAGE_CONNECTION_STRING%"
if ERRORLEVEL 1 goto :eof

goto :eof

:nobits
echo 'publish' folder not found, please run Solution/Publish first.
exit /b 1

:nokey
echo Please set your ADA_KIOSK_STORAGE_CONNECTION_STRING
exit /b 1
