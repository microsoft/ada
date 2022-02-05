# This script assumes you already have run the following:
#
#    az login
#    az account set --subscription id
#
# This script does NOT deploy the code, it just creates all the Azure resources we need to
# deploy the code.  Deploying the code is done after this script completes using Visual Studio Code.

$resource_group = "ada-server-rg"
$plan_location = "westus2"

$storage_account_name = "adaserverstorage"
$webpubsub = "AdaPubSubService"

Set-Location $PSScriptRoot

function Invoke-Tool($prompt, $command)
{
    $output = Invoke-Expression $command
    $ec = $LastExitCode
    if ($ec -ne 0)
    {
        Write-Host "### Error $ec running command $command" -ForegroundColor Red
        write-Host $output
        Exit $ec
    }
    return $output
}

function GetConnectionString()
{
    $x = Invoke-Tool -prompt "Get storage account connection string..." -command "az storage account show-connection-string --name $storage_account_name  --resource-group $resource_group" | ConvertFrom-Json
    return $x.connectionString
}

function GetStorageAccount()
{
    $output = &az storage account list --resource-group $resource_group 2>&1
    if ($output.ToString().Contains("could not be found")) {
        $storageAcct = $null
    } else {
        $storageInfo = $output | ConvertFrom-Json
        $storageAcct = $storageInfo | where-object name -eq $storage_account_name
    }
    return $storageAcct
}

function PrintJsonStatus($prompt, $obj, $name)
{
    $x = $obj | ConvertFrom-Json
    $p = $x | Get-Member -Name $name
    if ($null -ne $p){
        $value = $p.Definition.Split('=')[1]
        Write-Host $prompt " is " $value
    }
}

function Set-JToken($jobject, $name, $value)
{
    # adds property to Newtonsoft.Json.Linq.JObject
    $v = $jobject.GetValue($name)
    if ($null -eq $v){
        $jobject.Add($name, [Newtonsoft.Json.Linq.JValue]::CreateString($value))
    } else {
        $v.Value = $value
    }
}

Write-Host "Checking resource group $resource_group"
$output = &az group show --name $resource_group 2>&1
$ec = $LastExitCode
if ($ec -eq 3) {
    Write-Host "Resource group $resource_group not found and this script doesn't have permission to create it."
    Exit-PSSession
}
elseif ($ec -ne 0)
{
    Write-Host "### Error $ec looking for resource group $resource_group" -ForegroundColor Red
    write-Host $output
    Exit-PSSession
}

Write-Host "Check Web Pub Sub service..."
$output = &az webpubsub show --name $webpubsub --resource-group $resource_group 2>&1
$ec = $LastExitCode
if ($ec -eq 3) {
    Write-Host "Creating Web Pub Sub service..."
    $output = az webpubsub create --name $webpubsub --resource-group $resource_group --sku Standard_S1
}
$info = $output | ConvertFrom-Json
$webpubsub_hostname = $info.hostName
$webpubsub_connstr = &az webpubsub key show --name $webpubsub --resource-group $resource_group --query primaryConnectionString

# create storage account
Write-Host "Checking storage account..."
$storageAcct = GetStorageAccount
if ($null -eq $storageAcct) {
    $output = Invoke-Tool -prompt "Creating storage account..." -command "az storage account create --name $storage_account_name --resource-group $resource_group --location $plan_location --kind StorageV2 --sku Standard_LRS"
}

$storage_connection = GetConnectionString
Write-Host "Please set ADA_STORAGE_CONNECTION_STRING to the following connection string (without the double quotes)"
Write-Host $storage_connection

Write-Host ""

Write-Host "Please set ADA_WEBPUBSUB_CONNECTION_STRING to the following connection string (without the double quotes)"
Write-Host $webpubsub_connstr

Write-Host ""
Write-Host "====================================================================================================="
Write-Host "Azure resource group '$resource_group' setup complete."
Write-Host "====================================================================================================="
