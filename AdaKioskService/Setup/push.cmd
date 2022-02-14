REM open a network drive named z: to your AdaKiosk tablet Temp
REM folder where we will copy the bits to, from there you can
REM deploy the bits on the tablet to the c:\AdaKiosk folder.

if not exist z:\AdaKiosk mkdir z:\AdaKiosk
xcopy /y %~dp0..\bin\Release\net5.0-windows\*.* z:\AdaKiosk

if not exist z:\Scripts mkdir z:\Scripts
xcopy /y %~dp0\*.* z:\Scripts

if not exist z:\AdaKioskService mkdir z:\AdaKioskService
xcopy /y %~dp0..\..\AdaKioskService\bin\Release\*.* z:\AdaKioskService
