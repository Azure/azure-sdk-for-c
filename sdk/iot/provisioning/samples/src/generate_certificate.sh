#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

openssl ecparam -out device_ec_key.pem -name prime256v1 -genkey
openssl req -new -days 365 -nodes -x509 -key device_ec_key.pem -out device_ec_cert.pem -config x509_config.cfg -subj "/CN=paho-sample-device1"
openssl x509 -text -noout -fingerprint -in device_ec_cert.pem

rm device_cert_store.pem
cat device_ec_cert.pem device_ec_key.pem >> device_cert_store.pem

echo It is recommended to use the OpenSSL Trusted CA store configured on your system.
echo If required (for example on Windows), download the Baltimore PEM CA from https://www.digicert.com/digicert-root-certificates.htm to the current folder.
echo    export AZ_IOT_DEVICE_X509_TRUST_PEM_FILE=$(pwd)/BaltimoreCyberTrustRoot.crt.pem

echo Sample certificate generated:
echo Upload device_ec_cert.pem to Device Provisioning Service.
echo Use the device_cert_store.pem file within the sample:
echo    export AZ_IOT_DEVICE_X509_CERT_PEM_FILE=$(pwd)/device_cert_store.pem
