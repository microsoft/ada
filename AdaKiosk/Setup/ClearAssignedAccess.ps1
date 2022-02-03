# From  https://docs.microsoft.com/en-us/windows/client-management/mdm/using-powershell-scripting-with-the-wmi-bridge-provider
# and https://docs.microsoft.com/en-us/windows/win32/dmwmibridgeprov/mdm-assignedaccess
$nameSpaceName="root\cimv2\mdm\dmmap"
$className="MDM_AssignedAccess"
$obj = Get-CimInstance -Namespace $namespaceName -ClassName $className
Write-Host ($obj | Format-List | Out-String)
$obj.Configuration = $null
Set-CimInstance -CimInstance $obj