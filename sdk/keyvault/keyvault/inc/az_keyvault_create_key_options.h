// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_KEYVAULT_CREATE_KEY_OPTIONS_H
#define AZ_KEYVAULT_CREATE_KEY_OPTIONS_H

#include <az_optional_bool.h>
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_span.h>
#include <az_str.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_decrypt() { return AZ_STR("decrypt"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_encrypt() { return AZ_STR("encrypt"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_sign() { return AZ_STR("sign"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_unwrapKey() { return AZ_STR("unwrapKey"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_verify() { return AZ_STR("verify"); }
AZ_NODISCARD AZ_INLINE az_span az_keyvault_key_operation_wrapKey() { return AZ_STR("wrapKey"); }

typedef struct {
  az_optional_bool enabled;
  az_span_span_builder operations;
  az_pair_span_builder tags;
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
  return self->operations.length == 0;
}

/**
 * @brief check if operation array is full
 *
 */
AZ_NODISCARD AZ_INLINE bool az_keyvault_create_key_options_is_full(
    az_keyvault_create_key_options * const self) {
  return self->operations.buffer.size != 0
      && self->operations.length == self->operations.buffer.size;
}

/**
 * @brief set length of operations to 0 so it can be written again
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_keyvault_create_key_options_clear(az_keyvault_create_key_options * const self) {
  self->operations.length = 0;
  return AZ_OK;
}

/**
 * @brief Append an operation to a create key options
 *
 */
AZ_NODISCARD AZ_INLINE az_result az_keyvault_create_key_options_append_operation(
    az_keyvault_create_key_options * const self,
    az_span const operation) {

  return az_span_span_builder_append(&self->operations, operation);
}

/**
 * @brief Append all available operations for key options
 *
 */
AZ_NODISCARD AZ_INLINE az_result
az_keyvault_create_key_options_append_all_operations(az_keyvault_create_key_options * const self) {

  if (!az_keyvault_create_key_options_is_empty(self)) {
    // options must be empty to allow appending all operations
    return AZ_ERROR_BUFFER_OVERFLOW;
  }
  AZ_RETURN_IF_FAILED(
      az_keyvault_create_key_options_append_operation(self, az_keyvault_key_operation_decrypt()));
  AZ_RETURN_IF_FAILED(
      az_keyvault_create_key_options_append_operation(self, az_keyvault_key_operation_encrypt()));
  AZ_RETURN_IF_FAILED(
      az_keyvault_create_key_options_append_operation(self, az_keyvault_key_operation_sign()));
  AZ_RETURN_IF_FAILED(
      az_keyvault_create_key_options_append_operation(self, az_keyvault_key_operation_unwrapKey()));
  AZ_RETURN_IF_FAILED(
      az_keyvault_create_key_options_append_operation(self, az_keyvault_key_operation_verify()));
  AZ_RETURN_IF_FAILED(
      az_keyvault_create_key_options_append_operation(self, az_keyvault_key_operation_wrapKey()));

  return AZ_OK;
}

#include <_az_cfg_suffix.h>

#endif
