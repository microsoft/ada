@echo off
pushd %~dp0
msbuild /target:restore "/p:Platform=Any CPU" /p:Configuration=Release  AdaKioskService.sln
msbuild /p:Configuration=Release "/p:Platform=Any CPU" /p:Configuration=Release AdaKioskService.sln

set serviceZipfile=bin\AdaKioskService.zip
if exist %serviceZipfile% del %serviceZipfile%
powershell -c "Compress-Archive -Path .\bin\Release\* -DestinationPath %serviceZipfile%"
if ERRORLEVEL 1 goto :err_zip

popd
goto :eof


:err_zip
echo Error creating AdaKiosk.zip
popd
exit /b 1