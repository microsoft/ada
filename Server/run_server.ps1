$Host.UI.RawUI.WindowTitle = "Ada Server"
$addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") -And (-Not ($_.IPAddress.StartsWith("192."))) }
$local_addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") -And ($_.IPAddress.StartsWith("192.")) }
if ($addr -eq $null) {
    $addr = $local_addr
}
if ($local_addr.GetType().IsArray){
    $local_addr = $local_addr[0]
}
Write-Host Running Server on $local_addr
Write-Host Hosting Web site on on $addr
# ensure C:\Users\Administrator\.conda\envs\ada\ is in the path
&python ada_server.py --ip $local_addr --loop