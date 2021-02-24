#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# SPDX-License-Identifier: MIT

command -v xxd >/dev/null 2>&1 || { echo >&2 "Please install xxd."; exit 1; }

wget https://cacerts.digicert.com/BaltimoreCyberTrustRoot.crt.pem -O ca.pem
echo -n -e '\0' >> ca.pem
xxd -i ca.pem ca.h
