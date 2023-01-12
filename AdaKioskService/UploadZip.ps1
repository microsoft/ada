&msbuild /target:restore "/p:Platform=Any CPU" /p:Configuration=Release  AdaKioskService.sln
&msbuild /p:Configuration=Release "/p:Platform=Any CPU" /p:Configuration=Release AdaKioskService.sln

$bits = ".\bin\Release"
if (Test-Path "bin\AdaKioskService.zip") {
    Remove-Item "bin\AdaKioskService.zip"
}
Compress-Archive -Path $bits\* -DestinationPath bin\AdaKioskService.zip
$hash = Get-FileHash bin\AdaKioskService.zip -Algorithm SHA256
if (Test-Path "bin\AdaKioskService.zip.hash") {
    Remove-Item "bin\AdaKioskService.zip.hash"
}
Set-Content -Path "bin\AdaKioskService.zip.hash" -Value $hash

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

$rc = &az storage blob upload --file bin\AdaKioskService.zip --container adakiosk --connection-string %ADA_STORAGE_CONNECTION_STRING%  --overwrite
$rc = &az storage blob upload --file bin\AdaKioskService.zip.hash --container adakiosk --connection-string %ADA_STORAGE_CONNECTION_STRING%  --overwrite