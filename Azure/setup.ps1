# This script assumes you already have run the following:
#
#    az login
#    az account set --subscription id
#    pwsh -c "Install-Module Newtonsoft.Json"
#
# This script does NOT deploy the code, it just creates all the Azure resources we need to
# deploy the code.  Deploying the code is done after this script completes using Visual Studio Code.
Import-Module Newtonsoft.Json

$resource_group = "ada-server-rg"
$plan_location = "westus2"

$storage_account_name = "adaserverstorage"
$webpubsub = "AdaPubSubService"

# the azure function for doing 3d reconstruction
$functions_service_plan = "ada-server-plan"
$function_app_sku = "S1"
$function_app = "ada-server-functions"
$function_runtime = "dotnet"
$functions_version = "3"
$functions_runtime_version = "3.1"

Set-Location $PSScriptRoot

function Invoke-Tool($prompt, $command) {
    $output = Invoke-Expression $command
    $ec = $LastExitCode
    if ($ec -ne 0) {
        Write-Host "### Error $ec running command $command" -ForegroundColor Red
        write-Host $output
        Exit $ec
    }
    return $output
}

function GetConnectionString() {
    $x = Invoke-Tool -prompt "Get storage account connection string..." -command "az storage account show-connection-string --name $storage_account_name  --resource-group $resource_group" | ConvertFrom-Json
    return $x.connectionString
}

function GetApiKey() {
    $x = Read-Host "Enter the Api Key Guid "
    return $x
}

function GetStorageAccount() {
    $output = &az storage account list --resource-group $resource_group 2>&1
    if ($output.ToString().Contains("could not be found")) {
        $storageAcct = $null
    }
    else {
        $storageInfo = $output | ConvertFrom-Json
        $storageAcct = $storageInfo | where-object name -eq $storage_account_name
    }
    return $storageAcct
}

function PrintJsonStatus($prompt, $obj, $name) {
    $x = $obj | ConvertFrom-Json
    $p = $x | Get-Member -Name $name
    if ($null -ne $p) {
        $value = $p.Definition.Split('=')[1]
        Write-Host $prompt " is " $value
    }
}

function Set-JToken($jobject, $name, $value) {
    # adds property to Newtonsoft.Json.Linq.JObject
    $v = $jobject.GetValue($name)
    if ($null -eq $v) {
        $jobject.Add($name, [Newtonsoft.Json.Linq.JValue]::CreateString($value))
    }
    else {
        $v.Value = $value
    }
}
function AddHostSettings($filename, $name, $value) {
    Write-Host "Updating $filename"
    if (-not (Test-Path -Path $filename)) {
        $json = "{ ""IsEncrypted"": false, ""Values"": { } }";
    }
    else {
        $json = Get-Content -Path $filename
    }
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
elseif ($ec -ne 0) {
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
$webpubsub_connstr = &az webpubsub key show --name $webpubsub --resource-group $resource_group --query primaryConnectionString
$webpubsub_connstr = $webpubsub_connstr.Trim('"')

# create storage account
Write-Host "Checking storage account..."
$storageAcct = GetStorageAccount
if ($null -eq $storageAcct) {
    $output = Invoke-Tool -prompt "Creating storage account..." -command "az storage account create --name $storage_account_name --resource-group $resource_group --location $plan_location --kind StorageV2 --sku Standard_LRS"
}

# create function app service plan.
Write-Host "Checking function app service plan..."
$output = &az appservice plan show --name $functions_service_plan --resource-group $resource_group
if ($null -eq $output) {
    $output = Invoke-Tool -prompt "Creating functions app service plan $functions_service_plan (windows, sku $function_app_sku)..." -command "az appservice plan create --resource-group $resource_group --name $functions_service_plan --sku $function_app_sku --location $plan_location"
}

$output = $output | ConvertFrom-Json
if ($output.properties.status -ne "Ready") {
    Write-Host "### Service plan $functions_service_plan is not ready" -ForegroundColor Red
    write-Host $output
    Exit-PSSession
}

Write-Host "Checking function app setup..."
$output = &az functionapp show --name $function_app --resource-group $resource_group 2>&1
$ec = $LastExitCode
if ($ec -eq 3) {
    $output = Invoke-Tool -prompt "Creating function app $function_app..." -command "az functionapp create --resource-group $resource_group --plan $functions_service_plan --storage-account $storage_account_name --name $function_app --runtime $function_runtime --functions-version $functions_version --os-type Windows --runtime-version $functions_runtime_version --disable-app-insights --https-only"
}
elseif ($ec -ne 0) {
    Write-Host "### Error $ec looking for functionapp $function_app" -ForegroundColor Red
    write-Host $output
    Exit-PSSession
}

$output = $output | ConvertFrom-Json
if ($output.state -ne "Running") {
    Write-Host "### Function app $function_app is not running" -ForegroundColor Red
    write-Host $output
    Exit-PSSession
}

$storage_connection = GetConnectionString
$apiKey = GetApiKey()

$output = Invoke-Tool -prompt "Configure settings on '$function_app'..." -command "az functionapp config appsettings set --name $function_app --resource-group $resource_group --settings `"AdaWebPubSubConnectionString=$webpubsub_connstr`""
$output = Invoke-Tool -prompt "Configure settings on '$function_app'..." -command "az functionapp config appsettings set --name $function_app --resource-group $resource_group --settings `"AdaStorageConnectionString=$storage_connection`""
$output = Invoke-Tool -prompt "Configure settings on '$function_app'..." -command "az functionapp config appsettings set --name $function_app --resource-group $resource_group --settings `"AdaWebPubSubApiKey=$apiKey`""


Write-Host "copy strings to local.settings.json"
AddHostSettings -filename "..\AdaServerRelay\local.settings.json" -name AdaWebPubSubConnectionString -value $webpubsub_connstr
AddHostSettings -filename "..\AdaServerRelay\local.settings.json" -name AdaStorageConnectionString -value $storage_connection
AddHostSettings -filename "..\AdaServerRelay\local.settings.json" -name AdaWebPubSubApiKey -value $apiKey

Write-Host "Please set ADA_STORAGE_CONNECTION_STRING to the following connection string (without the double quotes)"
Write-Host $storage_connection
Write-Host "Please use VS Code to publish the AdaServerRelay to resource group $resource_group app service $app_service"

Write-Host ""

Write-Host "Please set ADA_WEBPUBSUB_CONNECTION_STRING to the following connection string (without the double quotes)"
Write-Host $webpubsub_connstr

Write-Host ""
Write-Host "====================================================================================================="
Write-Host "Azure resource group '$resource_group' setup complete."
Write-Host "====================================================================================================="
