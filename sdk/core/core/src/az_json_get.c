// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_json_string_private.h"
#include <az_json.h>


#include <_az_cfg.h>

AZ_NODISCARD bool az_json_pointer_token_eq_json_string(az_span pointer_token, az_span json_string) {
  az_span_reader pt_reader = az_span_reader_create(pointer_token);
  az_span_reader js_reader = az_span_reader_create(json_string);
  while (true) {
    uint32_t pt_c = { 0 };
    az_result const pt_result = az_span_reader_read_json_pointer_token_char(&pt_reader, &pt_c);
    uint32_t js_c = { 0 };
    az_result const js_result = az_span_reader_read_json_string_char(&js_reader, &js_c);
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

AZ_NODISCARD az_result az_json_parser_get_by_pointer_token(
    az_json_parser * self,
    az_span pointer_token,
    az_json_token * out_token) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out_token);

  switch (out_token->kind) {
    case AZ_JSON_TOKEN_ARRAY: {
      uint64_t i = { 0 };
      AZ_RETURN_IF_FAILED(az_span_to_uint64(pointer_token, &i));
      while (true) {
        AZ_RETURN_IF_FAILED(az_json_parser_parse_array_item(self, out_token));
        if (i == 0) {
          return AZ_OK;
        }
        --i;
        AZ_RETURN_IF_FAILED(az_json_parser_skip_children(self, *out_token));
      }
    }
    case AZ_JSON_TOKEN_OBJECT: {
      while (true) {
        az_json_token_member token_member = { 0 };
        AZ_RETURN_IF_FAILED(az_json_parser_parse_token_member(self, &token_member));
        if (az_json_pointer_token_eq_json_string(pointer_token, token_member.name)) {
          *out_token = token_member.token;
          return AZ_OK;
        }
        AZ_RETURN_IF_FAILED(az_json_parser_skip_children(self, token_member.token));
      }
    }
    default:
      return AZ_ERROR_ITEM_NOT_FOUND;
  }
}

AZ_NODISCARD az_result
az_json_parse_by_pointer(az_span json, az_span pointer, az_json_token * out_token) {
  AZ_CONTRACT_ARG_NOT_NULL(out_token);

  az_json_parser json_parser = { 0 };
  AZ_RETURN_IF_FAILED(az_json_parser_init(&json_parser, json));
  az_span_reader pointer_parser = az_span_reader_create(pointer);

  AZ_RETURN_IF_FAILED(az_json_parser_parse_token(&json_parser, out_token));

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
        az_json_parser_get_by_pointer_token(&json_parser, pointer_token, out_token));
  }
}
