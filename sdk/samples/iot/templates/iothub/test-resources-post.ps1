#!/usr/bin/env pwsh

[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param (
    [Parameter(Mandatory = $true)]
    [string] $ResourceGroupName,

    [Parameter()]
    [string] $TestApplicationOid,

    # The DeploymentOutputs parameter is only valid in the test-resources-post.ps1 script.
    [Parameter()]
    [hashtable] $DeploymentOutputs,

    # Captures any arguments from eng/New-TestResources.ps1 not declared here (no parameter errors).
    [Parameter(ValueFromRemainingArguments = $true)]
    $RemainingArguments
)

###### setup ######
Install-Module -Name Az -RequiredVersion 4.8.0 -Force -AllowClobber -SkipPublisherCheck

if ($IsLinux) {
  $module_location_prefix = "$HOME\.local\share\powershell\Modules"
  Invoke-Expression -Command "sudo apt install libssl-dev"
}
if ($IsWindows) {
  $module_location_prefix = "$HOME\Documents\PowerShell\Modules"
}
if ($IsMacOS) {
  $module_location_prefix = "$HOME\.local\share\powershell\Modules"
}

Import-Module -Name $module_location_prefix\Az.IotHub -Force

$orig_loc = Get-Location
Write-Host $orig_loc
Write-Host "##vso[task.setvariable variable=VCPKG_ROOT]:$orig_loc/vcpkg"
cd $orig_loc\sdk\samples\iot\
$sourcesDir = Get-Location

$region = $DeploymentOutputs['._LOCATION']
$deviceID = "aziotbld-c-sample"
$deviceIDSaS = "aziotbld-c-sample-sas"
$iothubName = $DeploymentOutputs['IOT_HUB_NAME']

###### X509 setup ######
# Generate certificate 
openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 12 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=$deviceID"

Get-Content -Path device_ec_cert.pem, device_ec_key.pem | Set-Content -Path device_cert_store.pem
openssl x509 -noout -fingerprint -in device_ec_cert.pem | % {$_.replace(":", "")} | % {$_.replace("SHA1 Fingerprint=", "")} | Tee-Object -FilePath fingerprint.txt
$fingerprint = Get-Content -Path .\fingerprint.txt

# Get the hub as an object
$hub_obj = Get-AzIotHub -ResourceGroupName $ResourceGroupName -Name $iothubName

# Pass fingerprint to IoTHub
Write-Host "Adding cert device to the allocated hub"
Add-AzIotHubDevice `
-InputObject $hub_obj `
-DeviceId $deviceID `
-AuthMethod "x509_thumbprint" `
-PrimaryThumbprint $fingerprint `
-SecondaryThumbprint $fingerprint

# Download Baltimore Cert
curl https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem > $sourcesDir\BaltimoreCyberTrustRoot.crt.pem

# Link IoTHub to DPS service
$hubConnectionString = Get-AzIotHubConnectionString -ResourceGroupName $ResourceGroupName -Name $iothubName -KeyName "iothubowner"

###### SaS setup ######
# Create IoT SaS Device
Write-Host "Adding SAS Key device to the allocated hub"
Add-AzIotHubDevice `
-ResourceGroupName $ResourceGroupName `
-IotHubName $iothubName `
-DeviceId $deviceIDSaS `
-AuthMethod "shared_private_key"

Write-Host "Getting connection string and adding environment variables"

$deviceSaSConnectionString = Get-AzIotHubDeviceConnectionString -ResourceGroupName $ResourceGroupName -IotHubName $iothubName -deviceId $deviceIDSaS

$sasKey = $deviceSaSConnectionString.ConnectionString.Split("SharedAccessKey=")[1]

$deviceCertPath = Join-Path $sourcesDir "device_cert_store.pem" -Resolve
$trustedCertPath = Join-Path $sourcesDir "BaltimoreCyberTrustRoot.crt.pem" -Resolve

# add env defines for IoT samples
Write-Host "##vso[task.setvariable variable=AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH]$deviceCertPath"
Write-Host "##vso[task.setvariable variable=AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH]$trustedCertPath"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_DEVICE_ID]$deviceID"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_HOSTNAME]$iothubName.azure-devices.net"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_SAS_DEVICE_ID]$deviceIDSaS"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_SAS_KEY]$sasKey"

Set-Location $orig_loc
