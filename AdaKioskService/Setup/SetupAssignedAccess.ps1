# From  https://docs.microsoft.com/en-us/windows/client-management/mdm/using-powershell-scripting-with-the-wmi-bridge-provider
# and https://docs.microsoft.com/en-us/windows/win32/dmwmibridgeprov/mdm-assignedaccess
$nameSpaceName="root\cimv2\mdm\dmmap"
$className="MDM_AssignedAccess"
$obj = Get-CimInstance -Namespace $namespaceName -ClassName $className
Write-Host "got instance $obj.Configuration"
Add-Type -AssemblyName System.Web
$xml = Get-Content -Path "AssignedAccessProfile.xml" -Encoding "utf8" | Out-String
$html = [System.Web.HttpUtility]::HtmlEncode($xml)
$obj.Configuration = $html
Set-CimInstance -CimInstance $obj

