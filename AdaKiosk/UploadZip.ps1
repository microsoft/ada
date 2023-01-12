&msbuild /target:restore AdaKiosk.sln
&msbuild /p:Configuration=Release "/p:Platform=Any CPU" AdaKiosk.sln

$bits = ".\bin\Release\net7.0-windows"
if (Test-Path "bin\AdaKiosk.zip") {
    Remove-Item "bin\AdaKiosk.zip"
}
Compress-Archive -Path $bits\* -DestinationPath bin\AdaKiosk.zip
$hash = Get-FileHash bin\AdaKiosk.zip -Algorithm SHA256
if (Test-Path "bin\AdaKiosk.zip.hash") {
    Remove-Item "bin\AdaKiosk.zip.hash"
}
Set-Content -Path "bin\AdaKiosk.zip.hash" -Value $hash.Hash

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