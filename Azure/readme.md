# Azure Setup

The Ada Server uses an [Azure Web Pub Sub](https://azure.microsoft.com/en-us/services/web-pubsub/) service
to communicate with the Ada Kiosk.

The `setup.ps1` script can be used to create the azure resources in your chosen subscription.

This also creates an Azure Function for running the
[AdaServerRelay](../AdaServerRelay.md).

```
pwsh -f setup.ps1
```

The resulting connection string needs to then be copied to an environment variable named
ADA_WEBPUBSUB_CONNECTION_STRING both on the Ada server machine and on the Kiosk machine
and any other machine you want to run unit tests on.

See [AdaWebPubSub](../AdaWebPubSub/readme.md) for information on how to send commands to
Ada using this service.