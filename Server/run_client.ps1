param (
    [Parameter(Mandatory=$true,
                HelpMessage="Name of the raspberry pi to connect to")]
    [string]$name   
)

# wait for DHCP to kick in.
$addr = $null
while (-not $addr) {
    $addr = Get-NetIPAddress -AddressFamily IPv4 | Where-Object -FilterScript { ($_.PrefixOrigin -Eq "Dhcp") }
}

$out = $null
while (-not $out) {
    $out = Test-Connection -ComputerName $name
    if ($out) {
        &ssh "pi@$name" -t "/home/pi/git/Ada/RpiController/run.sh"
    }
    Start-Sleep -s 10
}