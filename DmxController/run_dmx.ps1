$Host.UI.RawUI.WindowTitle = "DMX Controller"
$local_addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") -And ($_.IPAddress.StartsWith("192.")) }
Write-Host Connecting to Server on $local_addr
&C:\ProgramData\Anaconda3\envs\ada\python.exe dmx_control.py --ip $local_addr --port com4