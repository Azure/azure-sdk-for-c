// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_get.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_json_get_object_member(az_span const json, az_span const name, az_json_value * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);
  AZ_CONTRACT_ARG_VALID_SPAN(json);
  AZ_CONTRACT_ARG_VALID_SPAN(name);

  az_json_parser parser = az_json_parser_create(json);
  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_parser_get(&parser, &value));

  if (value.kind == AZ_JSON_VALUE_OBJECT) {
    az_json_member member;
    while (az_json_parser_get_object_member(&parser, &member) != AZ_ERROR_JSON_NO_MORE_ITEMS) {
      if (az_span_eq(member.name, name)) {
        *out_value = member.value;
        return AZ_OK;
      }
    }
  }

  return AZ_ERROR_JSON_NOT_FOUND;
}

AZ_NODISCARD az_result az_json_get_object_member_string(
    az_span const json,
    az_span const name,
    az_span * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_STRING) {
    return AZ_ERROR_JSON_NOT_FOUND;
  }

  *out_value = value.data.string;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_get_object_member_number(az_span const json, az_span const name, double * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_NUMBER) {
    return AZ_ERROR_JSON_NOT_FOUND;
  }

  *out_value = value.data.number;
  return AZ_OK;
}

AZ_NODISCARD az_result
az_json_get_object_member_boolean(az_span const json, az_span const name, bool * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_NUMBER) {
    return AZ_ERROR_JSON_NOT_FOUND;
  }

  *out_value = value.data.boolean;
  return AZ_OK;
}
