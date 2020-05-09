// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>
#include <az_precondition_internal.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token const* token, bool* out_value)
{

  _az_PRECONDITION_NOT_NULL(out_value);

  if (token->kind != AZ_JSON_TOKEN_TRUE && token->kind != AZ_JSON_TOKEN_FALSE)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out_value = token->_internal.boolean;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_token_get_string(az_json_token const* token, az_span* out_value)
{

  _az_PRECONDITION_NOT_NULL(out_value);

  if (token->kind != AZ_JSON_TOKEN_STRING)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out_value = token->_internal.string;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_token_get_number(az_json_token const* token, double* out_value)
{

  _az_PRECONDITION_NOT_NULL(out_value);

  if (token->kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out_value = token->_internal.number;
  return AZ_OK;
}
