// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_KEYVAULT_CLIENT_PRIVATE_H
#define _az_KEYVAULT_CLIENT_PRIVATE_H

#include <az_keyvault_create_key_options.h>
#include <az_result.h>
#include <az_span.h>

#include <stddef.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD az_result _az_keyvault_keys_key_create_build_json_body(
    az_span const json_web_key_type,
    az_keyvault_create_key_options const * const options,
    az_span_action const write);

#include <_az_cfg_suffix.h>

#endif
