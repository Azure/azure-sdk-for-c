// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_get.h>

#include <az_json_pointer_parser.h>
#include <az_json_string.h>

#include <_az_cfg.h>

AZ_NODISCARD bool az_json_pointer_token_eq_json_string(
    az_span const pointer_token,
    az_span const json_string) {
  az_span_reader pt_reader = az_span_reader_create(pointer_token);
  az_span_reader js_reader = az_span_reader_create(json_string);
  while (true) {
    uint8_t pt_c = { 0 };
    az_result const pt_result = az_span_reader_read_json_pointer_token_char(&pt_reader, &pt_c);
    uint16_t js_c = { 0 };
    az_result const js_result = az_span_reader_get_json_string_char(&js_reader, &js_c);
    if (js_result == AZ_ERROR_ITEM_NOT_FOUND && pt_result == AZ_ERROR_ITEM_NOT_FOUND) {
      return true;
    }
    if (az_failed(js_result) || az_failed(pt_result)) {
      return false;
    }
    if (pt_c != js_c) {
      return false;
    }
  }
}

AZ_NODISCARD az_result
az_json_get_object_member(az_span const json, az_span const name, az_json_value * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);
  AZ_CONTRACT_ARG_VALID_SPAN(json);
  AZ_CONTRACT_ARG_VALID_SPAN(name);

  az_json_parser parser = az_json_parser_create(json);
  az_json_value value = { 0 };
  AZ_RETURN_IF_FAILED(az_json_parser_get(&parser, &value));

  if (value.kind == AZ_JSON_VALUE_OBJECT) {
    while (true) {
      az_json_member member = { 0 };
      AZ_RETURN_IF_FAILED(az_json_parser_get_object_member(&parser, &member));
      if (az_span_eq(member.name, name)) {
        *out_value = member.value;
        return AZ_OK;
      }
    }
  }

  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD az_result az_json_parser_get_by_pointer_token(
    az_json_parser * const self,
    az_span const pointer_token,
    az_json_value * const p_value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(p_value);

  switch (p_value->kind) {
    case AZ_JSON_VALUE_ARRAY: {
      uint64_t i = { 0 };
      AZ_RETURN_IF_FAILED(az_span_get_uint64(pointer_token, &i));
      while (true) {
        AZ_RETURN_IF_FAILED(az_json_parser_get_array_element(self, p_value));
        if (i == 0) {
          return AZ_OK;
        }
        --i;
        AZ_RETURN_IF_FAILED(az_json_parser_skip(self, *p_value));
      }
    }
    case AZ_JSON_VALUE_OBJECT: {
      while (true) {
        az_json_member member = { 0 };
        AZ_RETURN_IF_FAILED(az_json_parser_get_object_member(self, &member));
        if (az_json_pointer_token_eq_json_string(pointer_token, member.name)) {
          *p_value = member.value;
          return AZ_OK;
        }
        AZ_RETURN_IF_FAILED(az_json_parser_skip(self, member.value));
      }
    }
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }
}

AZ_NODISCARD az_result
az_json_get_by_pointer(az_span const json, az_span const pointer, az_json_value * const out_value) {
  AZ_CONTRACT_ARG_NOT_NULL(out_value);

  az_json_parser json_parser = az_json_parser_create(json);
  az_span_reader pointer_parser = az_span_reader_create(pointer);

  AZ_RETURN_IF_FAILED(az_json_parser_get(&json_parser, out_value));

  while (true) {
    az_span pointer_token = { 0 };
    // read the pointer token.
    {
      az_result const result
          = az_span_reader_read_json_pointer_token(&pointer_parser, &pointer_token);
      // no more pointer tokens so we found the JSON value.
      if (result == AZ_ERROR_ITEM_NOT_FOUND) {
        return AZ_OK;
      }
      AZ_RETURN_IF_FAILED(result);
    }
    AZ_RETURN_IF_FAILED(
        az_json_parser_get_by_pointer_token(&json_parser, pointer_token, out_value));
  }
}
