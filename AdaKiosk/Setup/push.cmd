if not exist z:\AdaKiosk mkdir z:\AdaKiosk
xcopy /y %~dp0..\bin\Release\net5.0-windows\*.* z:\AdaKiosk

if not exist z:\Scripts mkdir z:\Scripts
xcopy /y %~dp0\*.* z:\Scripts

if not exist z:\AdaKioskService mkdir z:\AdaKioskService
xcopy /y %~dp0..\..\AdaKioskService\bin\Release\*.* z:\AdaKioskService
