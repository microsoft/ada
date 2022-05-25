$Host.UI.RawUI.WindowTitle = "DMX Controller"

# wait for DHCP to kick in.
$addr = $null
while (-not $addr) {
    $addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") }
}

# the server should be on a 192.* address.
$local_addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") -And ($_.IPAddress.StartsWith("192.")) }
if ($local_addr.GetType().IsArray){
    $local_addr = $local_addr[0]
}

Write-Host Connecting to Server on $local_addr
&C:\ProgramData\Anaconda3\envs\ada\python.exe dmx_control.py --ip $local_addr --port com4