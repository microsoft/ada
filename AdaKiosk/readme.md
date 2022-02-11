## AdaKiosk

This is a Windows WPF app that provides a nice Kiosk experience to
accompany Ada.

![kiosk](images/kiosk.png)

## Home page

The app comes up by default in full screen mode.  Press F11 to get out of full screen mode.

This page is the default and provides access to the beautiful blog on Ada detailing the design
concepts and construction effort.  This app connects to Ada via the very cool new [Azure Web Pub Sub service](https://azure.microsoft.com/en-us/services/web-pubsub/), and connects via an environment variable called ADA_WEBPUBSUB_CONNECTION_STRING for it to make this
connection.  It also auto-updates via the [AdaKioskService](../AdaKioskService/readme.md).

![image](images/home.png)

## Simulation

This page runs a visual simulation of what Ada is doing. There is an
advanced feature that lets you edit the zone_maps using this page. You
can also click on the kitchens to generate random color events. The
Azure Cosmos button turns on a simulation that generates random colors
for each kitchen zone every few seconds.

This page can also be used to edit the `zone_map_*.json` files. The
updated files will be saved in the bin/debug folder near the binary.
Press F8 to start editing zone maps, then click on a kitchen, and
press F3 to clear the zone map for that kitchen, then click on the
strips you want to belong to that zone.  Repeat for the other kitchens
and when you are finished type Ctrl+S to save the result. Press F5 to
clear the screen.

![image](images/simulation.png)

## Control

This page provides some simple controls over Ada so you override the Ada
default schedule with the On/Off buttons, you can set specific colors or
pick the specified emotion and you can run specific animations as defined
in the Server/config.json.

There is also a manual control at the bottom of the page for setting color
of a specific pi, strip and/or range of leds.  The led range can be empty
in which case it sets the whole strip or it can be a comma separate list of
led indexes, including an led range, like this "1,2-5,8,10-20"

![image](images/control.png)

## Debug

This is an advanced page for testing individual LED's on Ada.
This page is only made visible when a /debug/true message is sent
to the Kiosk.