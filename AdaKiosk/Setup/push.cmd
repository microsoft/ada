if not exist z:\AdaKiosk mkdir z:\AdaKiosk
xcopy /y %~dp0..\bin\Release\net5.0-windows\*.* z:\AdaKiosk
if not exist z:\AdaKioskUpdater mkdir z:\AdaKioskUpdater
xcopy /y %~dp0..\..\AdaKioskUpdater\bin\Release\net5.0\*.* z:\AdaKioskUpdater