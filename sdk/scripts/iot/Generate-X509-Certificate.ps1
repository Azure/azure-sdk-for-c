#!/usr/bin/env pwsh

param(
[Parameter(Mandatory = $true)] [string]$SOURCES_DIR,
[Parameter(Mandatory = $true)] [string]$AZURE_RESOURCEGROUP_NAME,
[Parameter(Mandatory = $true)] [string]$IOTHUB_NAME,
[Parameter(Mandatory = $true)] [string]$DEVICE_ID,
[Parameter(Mandatory = $true)] [string]$DPS_NAME

)

# Generate certificate 
$orig_loc = Get-Location
cd $SOURCES_DIR\sdk\samples\iot\
openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=$DEVICE_ID"
dir

Get-Content device_ec_cert.pem, device_ec_key.pem | Set-Content device_cert_store.pem
openssl x509 -noout -fingerprint -in device_ec_cert.pem | % {$_.replace(":", "")} | % {$_.replace("SHA1 Fingerprint=", "")} | Tee-Object fingerprint.txt
$fingerprint = Get-Content -Path .\fingerprint.txt

# Pass fingerprint to IoTHub 
Add-AzIotHubDevice `
-ResourceGroupName $AZURE_RESOURCEGROUP_NAME `
-IotHubName $IOTHUB_NAME `
-DeviceId $DEVICE_ID `
-AuthMethod "x509_thumbprint" `
-PrimaryThumbprint $fingerprint `
-SecondaryThumbprint $fingerprint

# Download Baltimore Cert
curl https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem > $SOURCES_DIR\sdk\samples\iot\BaltimoreCyberTrustRoot.crt.pem

# Link IoTHub to DPS service
$hubConnectionString=Get-AzIotHubConnectionString -ResourceGroupName $AZURE_RESOURCEGROUP_NAME -Name $IOTHUB_NAME -KeyName "iothubowner"
Add-AzIoTDeviceProvisioningServiceLinkedHub -ResourceGroupName $AZURE_RESOURCEGROUP_NAME -Name $DPS_NAME -IotHubConnectionString $hubConnectionString.PrimaryConnectionString --IotHubLocation eastus

Set-Location $orig_loc