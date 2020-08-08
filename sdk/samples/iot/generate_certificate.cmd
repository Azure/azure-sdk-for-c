@REM Copyright (c) Microsoft Corporation. All rights reserved.
@REM SPDX-License-Identifier: MIT

@echo off

openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
IF %ERRORLEVEL% NEQ 0 (
	echo "Failed generating certificate key"
	exit /b 1
)

openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
IF %ERRORLEVEL% NEQ 0 (
	echo "Failed generating certificate"
	exit /b 1
)

openssl x509 -noout -text -in device_ec_cert.pem

type device_ec_cert.pem > device_cert_store.pem
type device_ec_key.pem >> device_cert_store.pem

echo.
echo IMPORTANT:
echo It is NOT recommended to use OpenSSL on Windows or OSX. Recommended TLS stacks are:
echo Microsoft Windows SChannel: https://docs.microsoft.com/en-us/windows/win32/com/schannel
echo OR
echo Apple Secure Transport : https://developer.apple.com/documentation/security/secure_transport
echo If using OpenSSL, it is recommended to use the OpenSSL Trusted CA store configured on your system.
echo.
echo SAMPLE CERTIFICATE GENERATED:
echo Use the following command to set the environment variable for the samples:
echo.
echo		set AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH=%CD%\device_cert_store.pem
echo.
echo DPS SAMPLE:
echo Upload device_ec_cert.pem when enrolling your device with the Device Provisioning Service.
echo.
echo IOT HUB SAMPLES:
echo Use the following fingerprint when creating your device in IoT Hub.
echo (The fingerprint has also been placed in fingerprint.txt for future reference.)
echo.

FOR /F "tokens=*" %%a in ('openssl x509 -noout -fingerprint -in device_ec_cert.pem') do SET output=%%a
set fingerprint=%output::=%

echo 	%fingerprint%

echo %fingerprint% > fingerprint.txt
