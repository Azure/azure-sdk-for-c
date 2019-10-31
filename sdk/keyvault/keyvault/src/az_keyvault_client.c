// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_keyvault.h>

#include <az_contract.h>
#include <az_result.h>
#include <az_span_builder.h>
#include <az_span_seq.h>
#include <az_str.h>

#include <stdlib.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result 

// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_client_init(az_keyvault_keys_client * client, az_span uri, /*Azure Credentials */, az_keyvault_keys_client_options * options)


//TODO #define AZ_KEYVAULT_KEYS_KEYTYPE_XXXX
// Note: Options can be passed as NULL
//   results in default options being used
AZ_NODISCARD az_result az_keyvault_keys_createKey(az_keyvault_keys_client * client, az_span keyname, az_span (AZ_KEYVAULT_KEYS...) keytype, az_keyvault_keys_keys_options * options, az_http_response * out)
