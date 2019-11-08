// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_get.h>

#include <az_json_pointer_parser.h>

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

AZ_NODISCARD az_result az_json_parser_get_by_name(
    az_json_parser * const self,
    az_span const name,
    az_json_value * const p_value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(p_value);

  switch (p_value->kind) {
    case AZ_JSON_VALUE_ARRAY: {
      uint64_t i;
      AZ_RETURN_IF_FAILED(az_span_get_uint64(name, &i));
      while (true) {
        AZ_RETURN_IF_FAILED(az_json_parser_get_array_element(self, p_value));
        if (i == 0) {
          return AZ_OK;
        }
        i -= 1;
      }
    }
    case AZ_JSON_VALUE_OBJECT: {
      while (true) {
        az_json_member member;
        AZ_RETURN_IF_FAILED(az_json_parser_get_object_member(self, &member));
        // TODO: use parsers to compare JSON string and JSON pointer token.
        if (az_span_eq(name, member.name)) {
          *p_value = member.value;
          return AZ_OK;
        }
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
    az_span pointer_token;
    // read the pointer token.
    {
      az_result const result = az_json_pointer_parser_get(&pointer_parser, &pointer_token);
      // no more pointer tokens so we found the JSON value.
      if (result == AZ_ERROR_ITEM_NOT_FOUND) {
        return AZ_OK;
      }
      AZ_RETURN_IF_FAILED(result);
    }
    AZ_RETURN_IF_FAILED(az_json_parser_get_by_name(&json_parser, pointer_token, out_value));
  }
}
