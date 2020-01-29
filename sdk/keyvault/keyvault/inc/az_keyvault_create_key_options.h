// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_KEYVAULT_CREATE_KEY_OPTIONS_H
#define _az_KEYVAULT_CREATE_KEY_OPTIONS_H

#include <az_optional_types.h>

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_decrypt() {
  return AZ_SPAN_FROM_STR("decrypt");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_encrypt() {
  return AZ_SPAN_FROM_STR("encrypt");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_sign() { return AZ_SPAN_FROM_STR("sign"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_unwrapKey() {
  return AZ_SPAN_FROM_STR("unwrapKey");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_verify() {
  return AZ_SPAN_FROM_STR("verify");
}
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_wrapKey() {
  return AZ_SPAN_FROM_STR("wrapKey");
}

typedef struct {
  az_optional_bool enabled;
  az_span * operations;
  az_pair * tags;
  /* TODO: adding next options
  Datetime not_before;
  Datetime expires_on
  */
} az_keyvault_create_key_options;

/**
 * @brief check if there is at least one operation
 *
 */
AZ_NODISCARD AZ_INLINE bool az_keyvault_create_key_options_is_empty(
    az_keyvault_create_key_options const * self) {
  if (self == NULL) {
    return true;
  }
  az_span start = *(self->operations);
  return az_span_is_equal(start, az_span_null());
}

#include <_az_cfg_suffix.h>

#endif
