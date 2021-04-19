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

$repositoryRoot = Resolve-Path "$PSScriptRoot/../../../../.."
$sourcesDir = Resolve-Path "$repositoryRoot/sdk/samples/iot"
Push-Location $sourcesDir

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

$retryCount = 0
do
{
  $retryCount++
  Write-Host "AzIotHub is not yet active so sleeping for $retryCount seconds."
  Start-Sleep -Seconds $retryCount

  # Get the hub as an object
  $hub_obj = Get-AzIotHub -ResourceGroupName $ResourceGroupName -Name $iothubName
}
while ($retryCount -lt 5 -and $hub_obj.Properties.State -ne "Active")

if ($hub_obj.Properties.State -ne "Active")
{
  Write-Warning "AzIotHub instance is not yet active so later steps might fail as a result. If they fail often you can increase the retry count above."
}

# Pass fingerprint to IoTHub
Write-Host "Adding cert device to the allocated hub"
Add-AzIotHubDevice `
-InputObject $hub_obj `
-DeviceId $deviceID `
-AuthMethod "x509_thumbprint" `
-PrimaryThumbprint $fingerprint `
-SecondaryThumbprint $fingerprint

if ($LASTEXITCODE -ne 0)
{
  Write-Host "Adding cert device failed: LAST_ERROR_CODE=${LAST_ERROR_CODE}"
  exit $LASTEXITCODE
}

# Download Baltimore Cert
Write-Host "Downloading Baltimore root cert"
curl https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem > $sourcesDir\BaltimoreCyberTrustRoot.crt.pem

if ($LASTEXITCODE -ne 0)
{
  Write-Host "Downloading root cert failed: LAST_ERROR_CODE=${LAST_ERROR_CODE}"
  exit $LASTEXITCODE
}

###### SaS setup ######
# Create IoT SaS Device
Write-Host "Adding SAS Key device to the allocated hub"
Add-AzIotHubDevice `
-ResourceGroupName $ResourceGroupName `
-IotHubName $iothubName `
-DeviceId $deviceIDSaS `
-AuthMethod "shared_private_key"

if ($LASTEXITCODE -ne 0)
{
  Write-Host "Adding SAS key device failed: LAST_ERROR_CODE=${LAST_ERROR_CODE}"
  exit $LASTEXITCODE
}

Write-Host "Getting connection string for SAS device"

$deviceSaSConnectionString = Get-AzIotHubDeviceConnectionString -ResourceGroupName $ResourceGroupName -IotHubName $iothubName -deviceId $deviceIDSaS

if ($LASTEXITCODE -ne 0)
{
  Write-Host "Getting connection string for SAS device failed: LAST_ERROR_CODE=${LAST_ERROR_CODE}"
  exit $LASTEXITCODE
}

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

$vcpkgRoot = Join-Path $repositoryRoot vcpkg
Write-Host "##vso[task.setvariable variable=VCPKG_ROOT]:$vcpkgRoot"

Pop-Location
