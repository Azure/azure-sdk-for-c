// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>
#include <az_precondition_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token const* json_token, bool* out_value)
{
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_TRUE && json_token->kind != AZ_JSON_TOKEN_FALSE)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out_value = az_span_size(json_token->slice) == _az_STRING_LITERAL_LEN("true");
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_token_get_string(az_json_token const* json_token, az_span* out_value)
{
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind == AZ_JSON_TOKEN_ESCAPED_STRING
      || json_token->kind == AZ_JSON_TOKEN_ESCAPED_PROPERTY_NAME)
  {
    *out_value = json_token->slice;
    return AZ_OK;
  }
  else if (
      json_token->kind == AZ_JSON_TOKEN_STRING || json_token->kind == AZ_JSON_TOKEN_PROPERTY_NAME)
  {
    // TODO: Unescape and copy
    *out_value = json_token->slice;
    return AZ_OK;
  }
  else
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
}

AZ_NODISCARD az_result
az_json_token_get_uint64(az_json_token const* json_token, uint64_t* out_value)
{
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return az_span_atou64(json_token->slice, out_value);
}

AZ_NODISCARD az_result
az_json_token_get_uint32(az_json_token const* json_token, uint32_t* out_value)
{
  _az_PRECONDITION_NOT_NULL(out_value);

  if (json_token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return az_span_atou32(json_token->slice, out_value);
}
