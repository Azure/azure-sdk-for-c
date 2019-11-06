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
    while (true) {
      az_json_member member;
      AZ_RETURN_IF_FAILED(az_json_parser_get_object_member(&parser, &member));
      if (az_span_eq(member.name, name)) {
        *out_value = member.value;
        return AZ_OK;
      }
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_json_get_object_member_string(
    az_span const json,
    az_span const name,
    az_span * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_get_object_member(json, name, &value));

  if (value.kind != AZ_JSON_VALUE_STRING) {
    return AZ_ERROR_ITEM_NOT_FOUND;
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
    return AZ_ERROR_ITEM_NOT_FOUND;
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
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out_value = value.data.boolean;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_pointer_parser_get(az_span_reader * const self, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result
az_json_get_by_pointer(az_span const self, az_span const pointer, az_json_value * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_parser json_parser = az_json_parser_create(self);
  az_span_reader pointer_parser = az_span_reader_create(pointer);

  return AZ_ERROR_NOT_IMPLEMENTED;
}
