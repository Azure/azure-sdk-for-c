// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_KEYVAULT_KEY_BUNDLE_H
#define _az_KEYVAULT_KEY_BUNDLE_H

#include <az_mut_span.h>
#include <az_optional_bool.h>
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_span.h>
#include <az_str.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct {
  bool is_present;
  az_span data;
} az_span_optional;

typedef struct {
  az_span_optional key;
} az_keyvault_json_web_key;

typedef struct {
  bool is_present;
  az_keyvault_json_web_key data;
} az_keyvault_json_web_key_optional;

typedef struct {
  az_span recoveryLevel;
} az_keyvault_key_attributes;

typedef struct {
  bool is_present;
  az_keyvault_key_attributes data;
} az_keyvault_key_attributes_optional;

typedef struct {
  bool is_present;
  az_pair_span data;
} az_pair_span_optional;

typedef struct {
  az_keyvault_json_web_key_optional key;
  az_keyvault_key_attributes_optional attributes;
  az_pair_span_optional tags;
} az_keyvault_key_bundle;

AZ_NODISCARD az_result az_keyvault_json_to_key_bundle(
    az_span const json,
    az_mut_span const buffer,
    az_keyvault_key_bundle const ** const out);

#include <_az_cfg_suffix.h>

#endif
