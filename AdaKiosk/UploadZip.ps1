if (-not(Test-Path "bin\Release\net5.0-windows")) {
   Write-Host "Please build release version of AdaKiosk"
   Exit 1
}

if (Test-Path "bin\AdaKiosk.zip") {
    Remove-Item "bin\AdaKiosk.zip"
}
Compress-Archive -Path bin\Release\net5.0-windows\* -DestinationPath bin\AdaKiosk.zip
$hash = Get-FileHash bin\AdaKiosk.zip -Algorithm SHA256
if (Test-Path "bin\AdaKiosk.zip.hash") {
    Remove-Item "bin\AdaKiosk.zip.hash"
}
Set-Content -Path "bin\AdaKiosk.zip.hash" -Value $hash

$result = &az storage container list --connection-string %ADA_STORAGE_CONNECTION_STRING% | ConvertFrom-JSON
$found = $false
foreach ($item in $result) {
    if ($item.name -eq "adakiosk") {
        $found = $true
    }
}
if (-not($found)){   
   Write-Host "Please ensure storage account contains 'adakiosk' container".
   Exit 1
}

$rc = &az storage blob upload --file bin\AdaKiosk.zip --container adakiosk --connection-string %ADA_STORAGE_CONNECTION_STRING%  --overwrite
$rc = &az storage blob upload --file bin\AdaKiosk.zip.hash --container adakiosk --connection-string %ADA_STORAGE_CONNECTION_STRING%  --overwrite