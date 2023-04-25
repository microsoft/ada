## AdaServerRelay

This project provides a simple HTTP gateway to the `Azure Web Pub Sub` service used for providing
3 simple commands.  For the full Web Pub Sub interface see [AdaWebPubSub](../AdaWebPubSub/readme.md).

Setup for this Azure resource is included in
the `~/Azure/setup.ps1` script.

You can use the HTTP gateway to send these messages:

- /ping
- /bridge
- /kiosk/version/?

This relay can then be used in an Azure Logic App to ping the Ada Server every 15 minutes to make sure
it is still responding correctly and if a timeout occurs, send an email to someone to fix it.

![workflow](images/workflow.png)
