#!/usr/bin/env pwsh

param(
[Parameter(Mandatory = $true)] [string]$sourcesDir,
[Parameter(Mandatory = $true)] [string]$resourceGroupName,
[Parameter(Mandatory = $true)] [string]$iothubName,
[Parameter(Mandatory = $true)] [string]$deviceID,
[Parameter(Mandatory = $true)] [string]$dpsName,
[Parameter]	                   [string]$location
)

###### setup ######
#Uninstall-AzureRm
Install-Module -Name Az -RequiredVersion 4.8.0 -Force -AllowClobber
Install-Module -Name Az.DeviceProvisioningServices -Force
$orig_loc = Get-Location
cd $sourcesDir\sdk\samples\iot\
$deviceIDSaS = "aziotbld-c-sample-sas"

#debug
Write-Host (Get-AzAccount).SubscriptionName

$loc = if ($location) { $location } else { 'westus2' } 

###### X509 setup ######
# Generate certificate 
openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 20 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=$deviceID"

Get-Content -Path device_ec_cert.pem, device_ec_key.pem | Set-Content -Path device_cert_store.pem
openssl x509 -noout -fingerprint -in device_ec_cert.pem | % {$_.replace(":", "")} | % {$_.replace("SHA1 Fingerprint=", "")} | Tee-Object fingerprint.txt
$fingerprint = Get-Content -Path .\fingerprint.txt

# Pass fingerprint to IoTHub 
Add-AzIotHubDevice `
-ResourceGroupName $resourceGroupName `
-IotHubName $iothubName `
-DeviceId $deviceID `
-AuthMethod "x509_thumbprint" `
-PrimaryThumbprint $fingerprint `
-SecondaryThumbprint $fingerprint

# Download Baltimore Cert
curl https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem > $orig_loc\BaltimoreCyberTrustRoot.crt.pem #$sourcesDir\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem

# Link IoTHub to DPS service
$hubConnectionString=Get-AzIotHubConnectionString -ResourceGroupName $resourceGroupName -Name $iothubName -KeyName "iothubowner"
Add-AzIoTDeviceProvisioningServiceLinkedHub -ResourceGroupName $resourceGroupName -Name $dpsName -IotHubConnectionString $hubConnectionString.PrimaryConnectionString -IotHubLocation $loc

###### SaS setup ######
# Create IoT SaS Device 
Add-AzIotHubDevice `
-ResourceGroupName $resourceGroupName `
-IotHubName $iothubName `
-DeviceId $deviceIDSaS `
-AuthMethod "shared_private_key" 

# add env defines for IoT samples 
Write-Host "##vso[task.setvariable variable=AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH]:$sourcesDir\sdk\samples\iot\cert.pem"
Write-Host "##vso[task.setvariable variable=AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH]:$sourcesDir\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_DEVICE_ID]:aziotbld-c-sample"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_HOSTNAME]:aziotbld-embed-cd"


Set-Location $orig_loc
