#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

echo "Generating Validation Cert"
echo "DO NOT USE THIS IN PRODUCTION"
openssl ecparam -out validation_ec_key.pem -name prime256v1 -genkey
openssl req -new -key validation_ec_key.pem -out validation_ec.csr -subj "/CN=${1}"
openssl x509 -req -in validation_ec.csr -CA user_root_cert.pem -CAkey user_root_cert_key.pem -CAcreateserial -out validation_ec_cert.pem -extensions client_auth -extfile ./x509_config.cfg -days 30 -sha256
