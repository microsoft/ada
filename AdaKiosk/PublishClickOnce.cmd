@echo off
cd %~dp0
SET ROOT=%~dp0
set BITS=.\bin\publish

if "%ADA_STORAGE_CONNECTION_STRING%x" == "x" goto :noconnection
for /f "usebackq" %%i in (`tools\xsl -e -s Version\version.xsl Version\version.props`) do (
    set VERSION=%%i
)
echo ### Publishing version %VERSION%...

%ROOT%\tools\UpdateVersion.exe %VERSION% %ROOT%\Properties\PublishProfiles\ClickOnceProfile.pubxml

msbuild /target:restore AdaKiosk.sln
if ERRORLEVEL 1 goto :err_restore

msbuild /p:Configuration=Release "/p:Platform=Any CPU" AdaKiosk.sln
if ERRORLEVEL 1 goto :err_build

msbuild /target:publish /p:PublishProfile=.\Properties\PublishProfiles\ClickOnceProfile.pubxml AdaKiosk.csproj /p:PublishDir=.\bin\publish /p:Configuration=Release "/p:Platform=Any CPU"
if ERRORLEVEL 1 goto :err_publish

if not EXIST "%BITS%\AdaKiosk.application" goto :nobits
if not EXIST "%BITS%\setup.exe" goto :nobits

%ROOT%\tools\CleanupPublishFolder.exe %VERSION% bin\publish

echo "Please check contents of %BITS%..."
pause

call AzurePublishClickOnce.cmd bin\publish adakiosk/ClickOnce "%ADA_STORAGE_CONNECTION_STRING%"
if ERRORLEVEL 1 goto :eof

pwsh -f UploadZip.ps1

goto :eof

:err_restore
echo nuget restore failed
exit /b 1

:err_build
echo msbuild failed
exit /b 1

:err_publish
popd
echo dotnet publish failed
exit /b 1

:nobits
echo %BITS% folder seems incomplete, please check publish step.
goto :eof

:noconnection
echo Please set ADA_STORAGE_CONNECTION_STRING