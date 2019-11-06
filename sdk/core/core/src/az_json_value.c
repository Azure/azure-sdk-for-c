// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_value.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_json_value_get_boolean(az_json_value const * const self, bool * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  if (self->kind != AZ_JSON_VALUE_BOOLEAN) {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out = self->data.boolean;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_value_get_string(az_json_value const* const self, az_span* const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  if (self->kind != AZ_JSON_VALUE_STRING) {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out = self->data.string;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_value_get_number(az_json_value const* const self, double* const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  if (self->kind != AZ_JSON_VALUE_NUMBER) {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out = self->data.number;
  return AZ_OK;
}
