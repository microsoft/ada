
while ($True) {
    $addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") -And (-Not ($_.IPAddress.StartsWith("192."))) }
    if ($addr -ne $Nul) {
         $ip = $addr.IPAddress
         Write-Host "Found network address: $ip"
         Exit 0
    }
    Write-Host "Sleeping 10 seconds waiting for network..."
    Sleep 10
}
