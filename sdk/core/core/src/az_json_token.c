// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json.h>
#include <az_precondition_failed.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token self, bool* out)
{
  AZ_PRECONDITION_NOT_NULL(out);

  if (self.kind != AZ_JSON_TOKEN_BOOLEAN)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out = self.value.boolean;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_token_get_string(az_json_token self, az_span* out)
{
  AZ_PRECONDITION_NOT_NULL(out);

  if (self.kind != AZ_JSON_TOKEN_STRING)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out = self.value.string;
  return AZ_OK;
}

AZ_NODISCARD az_result az_json_token_get_number(az_json_token self, double* out)
{
  AZ_PRECONDITION_NOT_NULL(out);

  if (self.kind != AZ_JSON_TOKEN_NUMBER)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  *out = self.value.number;
  return AZ_OK;
}
