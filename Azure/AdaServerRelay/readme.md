## AdaServerRelay

This project provides a `SignalR` relay service for instant messaging between various Ada components.

See [https://ada-server-functions.azurewebsites.net/api/ada/home](https://ada-server-functions.azurewebsites.net/api/ada/home)

Open multiple instances and enter a message, you will see it replicated instantly across all instances.

The `AdaServer` uses this as a message bus to both publish what it is doing and also to receive direct user commands.