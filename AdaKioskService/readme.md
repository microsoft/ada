# AdaKioskService

This system service is running on the Ada Kiosk Tablet device in the background and
monitors an Azure Blob Store for updated Kiosk app binaries.  It connects to Azure
via a system environment variable named ADA_STORAGE_CONNECTION_STRING.

This service runs in the background as a system service on the Ada Kiosk Tablet device.
It connects to Azure looking for updates to the AdaKiosk app binaries.  If a new
version is found it downloads the new Zip file, unzips it, terminates the AdaKiosk
app to unlock the binaries, unzips the new bits in place, and reboots the tablet
so that it comes back up running the new AdaKiosk App.

You can use the [AdaKioskUnitTest.exe](../AdaKioskUnitTest/readme.md) to ping the
AdaKiosk from anywhere with the message "/kiosk/version/?" to see what version
is currently running. You should get a response like this:

```json
{"type":"message","from":"group","fromUserId":"kiosk","group":"demogroup","dataType":"json","data": "/kiosk/version/1.0.0.29"}
```

This message is also sent when the AdaKiosk app first launches, which means
if you are running the `AdaKioskUnitTest.exe` you will see the "auto-update"
happen when you publish new bits to Azure.

# Setup the AdaKioskService

To install the AdaKioskService on your Tablet device, start by creating
a shared folder named `\\adatablet\temp` that you can connect to over the
network using:

```
net use z: \\adatablet\temp
```

Then build the AdaKiosk.sln and the AdaKioskService solution and run
the  `Setup\push.cmd` script, to push the bits to the above Temp folder
on the Ada Kiosk Tablet.  Then remote into the tablet using an
Administrator account and run:

```
c:
mkdir c:\AdaKiosk\AdaKioskService
cd c:\AdaKiosk\AdaKioskService
xcopy /s c:\temp\AdaKioskService\*.* .
C:\Windows\Microsoft.NET\Framework\v4.0.30319\InstallUtil.exe AdaKioskService.exe
```

This will install the service.  The service will auto-start on reboot, but you
can also manually start it and stop it using the Windows Computer Management
app under "Services".  You will need to Stop it to update the service binaries.

# Publish binaries to Azure

To publish the AdaKiosk binaries to Azure for the AdaKioskService to find run build the Release
version of the [AdaKiosk App](../AdaKiosk/readme.md) and run the python script
`Scripts\PublishBinaries.py`

# First time setup of the AdaKiosk Tablet

First the AdaKiosk Tablet must be running windows 10 and not windows 11. Windows 11 has broken the
Multi-App Assigned Access feature we need to use so you need to stop your device from auto-updating
to Windows 11.  You can do that with the Group Policy Editor trick [shown
here](https://www.howtogeek.com/765377/how-to-block-the-windows-11-update-from-installing-on-windows-10/#autotoc_anchor_2).
This removes all the prompts telling you to update to Windows 11.

The AdaKiosk app connects to Ada via the [Azure Web Pub Sub
service](https://azure.microsoft.com/en-us/services/web-pubsub/) using an environment variable named
ADA_WEBPUBSUB_CONNECTION_STRING.  So you will need to define this as a system wide variable.

You will want the AdaKiosk app to auto-launch in a locked down user account on the AdaTablet device.
To achieve this, create a non-admin account on the tablet named `Ada`.  You will enable "Auto Logon"
for this account, so that the tablet comes up in Kiosk mode immediately on reboot.

Note that you can’t do that if Windows Hello is enabled, so remove any Windows Hello Pin from the
tablet, then you can setup this new account for auto login by using `NETPlwiz` as [shown
here](https://www.techjunkie.com/setup-auto-login-windows-10/). Once auto-login is working, all you
need is to setup multi-app assigned access, which will lock down the device and restrict allowed
apps and make it run automatically on reboot.

This is achieved with the [AssignedAccessProfile.xml](Setup\AssignedAccessProfile.xml)
profile which configures a Windows 10 feature called [Multi-app
AssignedAccess](https://docs.microsoft.com/en-us/windows/configuration/lock-down-windows-10-to-specific-apps).
But first we need to place the AdaKiosk binaries in the right place as follows:

```
mkdir c:\AdaKiosk\App
cd c:\AdaKiosk\App
xcopy /s c:\temp\AdaKiosk\*.* .
```

Now open the `c:\AdaKiosk\App` folder and make a shortcut for `AdaKiosk.exe` named
`AdaKioskShortcut.lnk` using the Windows Explorer, right click, "Create shortcut"

Make sure the App runs by running the `AdaKiosk.exe` there.  You may need to install .NET 5.0
runtime if it is not already installed on the machine.

Next, install the [PSTools](https://docs.microsoft.com/en-us/sysinternals/downloads/pstools) on the
tablet and run this from an Admin command prompt:

```
psexec.exe -i -s cmd.exe
```

This opens another command prompt, from there you can run the following:

```
cd c:\temp\Scripts
powershell -f SetupAssignedAccess.ps1
```

That's it!  You should now be able to reboot the tablet and have it autostart the `AdaKiosk.exe` app
in locked down mode so you can place the device out there in the real world to complete the Ada
show.

## Reset

If you ever want to "undo" this Kiosk mode lock down in your "Ada" user account you can run
`powershell -f ClearAssignedAccess.ps1` also from the `psexec.exe -i -s cmd.exe` command prompt.

