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

# the web app for html UI
# $webapp_service_plan = "ada-server-webappplan"
# $web_app_sku = "S1"
# $app_service = "ada-server"

# signal R service for sending output logs back to web clients.
$signalr_service_name = "ada-server-signalr"
$signalr_sku = "Standard_S1"
$signalr_unit_count = 1 # Each unit supports up to 1000 client connections.

# the azure function for doing 3d reconstruction
$functions_service_plan = "ada-server-plan"
$function_app_sku = "P1V2"
$function_app = "ada-server-functions"
$function_runtime = "dotnet"
$functions_version = 3

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

function GetSignalRConnectionString()
{
    $key = &az signalr key list --name $signalr_service_name --resource-group $resource_group --query primaryConnectionString -o json | ConvertFrom-Json
    return $key
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

function AddHostSettings($filename, $name, $value)
{
    Write-Host "Updating $filename"
    $json = Get-Content -Path $filename
    $hostsettings = [Newtonsoft.Json.JsonConvert]::DeserializeObject($json)
    $values = $hostsettings.GetValue("Values")
    Set-JToken -jobject $values -name $name -value $value
    $json = [Newtonsoft.Json.JsonConvert]::SerializeObject($hostsettings, [Newtonsoft.Json.Formatting]::Indented)
    Set-Content -Path "$filename" -Value $json
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

# create storage account
Write-Host "Checking storage account..."
$storageAcct = GetStorageAccount
if ($null -eq $storageAcct) {
    $output = Invoke-Tool -prompt "Creating storage account..." -command "az storage account create --name $storage_account_name --resource-group $resource_group --location $plan_location --kind StorageV2 --sku Standard_LRS"
}

# create premium service plan (needed to give Function App larger timeouts up to 30 minutes, and more CPU power to get the reconstruction job done)
$output = Invoke-Tool -prompt "Checking functions app service plan..." -command "az appservice plan show --name $functions_service_plan --resource-group $resource_group"
if ($null -eq $output)
{
    $output  = Invoke-Tool -prompt "Creating functions app service plan $functions_service_plan (windows, sku $function_app_sku)..." -command "az appservice plan create --resource-group $resource_group --name $functions_service_plan --sku $function_app_sku --location $plan_location"
} else {
    PrintJsonStatus -prompt "Service plan $functions_service_plan" -obj $output -name "status"
}

# create signalr R service
Write-Host "Checking signal R service..."
$output = &az signalr show --name $signalr_service_name --resource-group $resource_group 2>&1
$ec = $LastExitCode
if ($output.ToString().Contains("ResourceNotFound"))
{
    # if this fails with MissingSubscriptionRegistration see ​​https://docs.microsoft.com/en-us/azure/azure-resource-manager/templates/error-register-resource-provider
    $output = Invoke-Tool -prompt "Creating signal R service: $signalr_service_name ..." -command "az signalr create --resource-group $resource_group --name $signalr_service_name --sku $signalr_sku --unit-count $signalr_unit_count --service-mode Serverless --location $plan_location"
}
elseif ($ec -ne 0)
{
    Write-Host "### Error $ec looking for signalr service  '$signalr_service_name' in resource group $resource_group" -ForegroundColor Red
    write-Host $output
    Exit-PSSession
} else {
    PrintJsonStatus -prompt "SignalR service $signalr_service_name" -obj $output -name "provisioningState"
}

$signalr_connection_string = GetSignalRConnectionString

# create the 3dr function app
Write-Host "Checking function app setup..."
$output = &az functionapp show --name $function_app --resource-group $resource_group 2>&1
$ec = $LastExitCode
if ($ec -eq 3)
{
    $output = Invoke-Tool -prompt "Creating 3dr function app $function_app..." -command "az functionapp create --resource-group $resource_group --plan $functions_service_plan --storage-account $storage_account_name --name $function_app --runtime $function_runtime --functions-version $functions_version "
}
elseif ($ec -ne 0)
{
    Write-Host "### Error $ec looking for function app '$function_app' in resource group $resource_group" -ForegroundColor Red
    write-Host $output
    Exit-PSSession
} else {
    PrintJsonStatus -prompt "Function $function_app" -obj $output -name "state"
}

$output = Invoke-Tool -prompt "Configure settings on '$function_app'..." -command "az functionapp config appsettings set --name $function_app --resource-group $resource_group --settings `"AzureSignalRConnectionString=$signalr_connection_string`""

Write-Host "copy strings to local.settings.json"
AddHostSettings -filename "local.settings.json" -name AzureSignalRConnectionString -value $signalr_connection_string

Write-Host ""
Write-Host "====================================================================================================="
Write-Host "Azure resource group '$resource_group' setup complete."
Write-Host "Please use VS Code to publish the project to resource group $resource_group app service $app_service"
Write-Host "====================================================================================================="
