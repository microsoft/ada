
$setup = [System.IO.Path]::GetDirectoryName($PSScriptRoot)
$root = [System.IO.Path]::GetDirectoryName($setup)
Write-Host "Installing to $root\AdaKioskService"

&net stop AdaKioskService


try {    
    if (Test-Path -Path "$root\AdaKioskService") {
        # rename will fail if files are locked.
        Rename-Item "$root\AdaKioskService" "$root\AdaKioskServiceOld"
        Remove-Item "$root\AdaKioskServiceOld" -recurse
    }

    Rename-Item "$setup" "$root\AdaKioskService"
} catch {
    $e = $_
    if (-not (Test-Path -Path "$root\adakioskservice.log")) {
        Set-Content -Path "$root\adakioskservice.log" -Value "Logfile"
    }
    Add-Content -Path "$root\adakioskservice.log" -Value "Error updating service: $e"
}

&net start AdaKioskService
