#!/usr/bin/env pwsh

param(
[hashtable] $DeploymentOutputs
)

# setup
#Uninstall-AzureRm -Force
Install-Module -Name Az.DeviceProvisioningServices -Force
$orig_loc = Get-Location
Write-Host $orig_loc
#Write-Host "##vso[task.setvariable variable=VCPKG_DEFAULT_TRIPLET]:x64-windows-static"
Write-Host "##vso[task.setvariable variable=VCPKG_ROOT]:Get-Location"
cd $orig_loc\sdk\samples\iot\
$sourcesDir = Get-Location

$resourceGroupName = $DeploymentOutputs['._RESOURCE_GROUP']
$region = $DeploymentOutputs['._LOCATION']
$deviceID = "aziotbld-c-sample"
$deviceIDSaS = "aziotbld-c-sample-sas"
$dpsName = "aziotbld-c-dps"
$iothubName = "aziotbld-embed-cd"

###### X509 setup ######
# Generate certificate 
openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 12 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=$deviceID"

Get-Content device_ec_cert.pem, device_ec_key.pem | Set-Content device_cert_store.pem
openssl x509 -noout -fingerprint -in device_ec_cert.pem | % {$_.replace(":", "")} | % {$_.replace("SHA1 Fingerprint=", "")} | Tee-Object fingerprint.txt
$fingerprint = Get-Content -Path .\fingerprint.txt

# sleep, wait for IoTHub to deploy
Start-Sleep -s 60

# Pass fingerprint to IoTHub 
$deviceStatus = Retry{ Add-AzIotHubDevice `
-ResourceGroupName $resourceGroupName `
-IotHubName $iothubName `
-DeviceId $deviceID `
-AuthMethod "x509_thumbprint" `
-PrimaryThumbprint $fingerprint `
-SecondaryThumbprint $fingerprint }

# Download Baltimore Cert
curl https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem > $sourcesDir\BaltimoreCyberTrustRoot.crt.pem

# Link IoTHub to DPS service
$hubConnectionString=Get-AzIotHubConnectionString -ResourceGroupName $resourceGroupName -Name $iothubName -KeyName "iothubowner"
Add-AzIoTDeviceProvisioningServiceLinkedHub -ResourceGroupName $resourceGroupName -Name $dpsName -IotHubConnectionString $hubConnectionString.PrimaryConnectionString --IotHubLocation $region

###### SaS setup ######
# Create IoT SaS Device 
Add-AzIotHubDevice `
-ResourceGroupName $resourceGroupName `
-IotHubName $iothubName `
-DeviceId $deviceIDSaS `
-AuthMethod "shared_private_key" 

$deviceSaSConnectionString=Get-AzIotHubDeviceConnectionString -ResourceGroupName $resourceGroupName -IotHubName $iothubName -deviceId $deviceIDSaS -KeyName "Primary"

# add env defines for IoT samples 
Write-Host "##vso[task.setvariable variable=AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH]:$sourcesDir\cert.pem"
Write-Host "##vso[task.setvariable variable=AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH]:$sourcesDir\BaltimoreCyberTrustRoot.crt.pem"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_DEVICE_ID]:aziotbld-c-sample"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_HOSTNAME]:aziotbld-embed-cd"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_SAS_DEVICE_ID]:$deviceIDSaS"
Write-Host "##vso[task.setvariable variable=AZ_IOT_HUB_SAS_KEY]:$deviceSaSConnectionString.ConnectionString"

Set-Location $orig_loc
