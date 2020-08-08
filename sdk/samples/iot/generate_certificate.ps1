# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
openssl x509 -noout -text -in device_ec_cert.pem

if(Test-Path device_cert_store.pem)
{
  Remove-Item device_cert_store.pem
}

Get-Content device_ec_cert.pem, device_ec_key.pem | Set-Content device_cert_store.pem

Write-Output "IMPORTANT:"
Write-Output "It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:"
Write-Output "Microsoft Windows SChannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel"
Write-Output "OR"
Write-Output "Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport"
Write-Output "If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system."

Write-Output "`nSAMPLE CERTIFICATE GENERATED:"
Write-Output "Use the following command to set the environment variable for the samples:"
if ($IsWindows) {
  Write-Output "set AZ_IOT_DEVICE_X509_CERT_PEM_FILE=$(Resolve-Path device_cert_store.pem)"
}
else {
  Write-Output "export AZ_IOT_DEVICE_X509_CERT_PEM_FILE=$(Resolve-Path device_cert_store.pem)"
}

Write-Output "`nDPS SAMPLE:"
Write-Output "Upload device_ec_cert.pem when enrolling your device with the Device Provisioning Service."

Write-Output "`nIOT HUB SAMPLES:"
Write-Output "Use the following fingerprint when creating your device in IoT Hub."
Write-Output "(The fingerprint has also been placed in fingerprint.txt for future reference.)"
$fingerprint = openssl x509 -noout -fingerprint -in device_ec_cert.pem
$fingerprint -replace ':','' | Tee-Object fingerprint.txt
Write-Output " "