// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_builder.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_json_builder_init(az_json_builder * const out, az_mut_span const buffer) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  (void)buffer;

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_json_builder_result(az_json_builder const self, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  (void)self;

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result
az_json_builder_write(az_json_builder * const self, az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  (void)value;

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result
az_json_builder_write_object_member(az_json_builder * const self, az_json_member const member) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  (void)member;

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_json_builder_write_object_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result
az_json_builder_write_array_item(az_json_builder * const self, az_json_value const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  (void)value;

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_json_builder_write_array_close(az_json_builder * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  return AZ_ERROR_NOT_IMPLEMENTED;
}
