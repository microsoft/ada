$Host.UI.RawUI.WindowTitle = "Ada Server"
# ensure C:\Users\Administrator\.conda\envs\ada\ is in the path
while ($true) {
    &python ada_server.py --loop --one-day
    Start-Sleep -Seconds 2
}