# AdaKioskUnitTest

This is a simple C# app that connects to the Azure Web Pub Sub service
and prints out all messages being sent between the AdaKiosk
and the Ada Server.  You can also send your own adhoc messages from
the command line, for example, to find out what version of AdaKiosk
is running send this message `/kiosk/version/?` and the AdaKiosk 
should response with this:

```json
{"type":"message","from":"group","fromUserId":"kiosk","group":"demogroup","dataType":"json","data": "/kiosk/version/1.0.0.29"}
```

