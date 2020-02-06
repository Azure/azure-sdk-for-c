// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_reader_private.h"
#include <az_json.h>

#include <_az_cfg.h>

AZ_NODISCARD static az_result az_span_reader_read_json_pointer_char(
    az_span_reader * self,
    uint32_t * out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_result_byte const result = az_span_reader_current(self);
  switch (result) {
    case AZ_ERROR_EOF: {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    case '/': {
      return AZ_ERROR_JSON_POINTER_TOKEN_END;
    }
    case '~': {
      az_span_reader_next(self);
      az_result_byte const e = az_span_reader_current(self);
      az_span_reader_next(self);
      switch (e) {
        case '0': {
          *out = '~';
          return AZ_OK;
        }
        case '1': {
          *out = '/';
          return AZ_OK;
        }
        default: {
          return az_error_unexpected_char(e);
        }
      }
    }
    default: {
      az_span_reader_next(self);
      *out = (uint8_t)result;
      return AZ_OK;
    }
  }
}

AZ_NODISCARD az_result
az_span_reader_read_json_pointer_token(az_span_reader * self, az_span * out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // read `/` if any.
  {
    az_result result = az_span_reader_expect_char(self, '/');
    if (result == AZ_ERROR_EOF) {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    AZ_RETURN_IF_FAILED(result);
  }

  size_t const begin = self->i;
  while (true) {
    uint32_t ignore = { 0 };
    az_result result = az_span_reader_read_json_pointer_char(self, &ignore);
    switch (result) {
      case AZ_ERROR_ITEM_NOT_FOUND:
      case AZ_ERROR_JSON_POINTER_TOKEN_END: {
        AZ_RETURN_IF_FAILED(az_span_slice(self->span, begin, self->i, out));
        return AZ_OK;
      }
      default: {
        AZ_RETURN_IF_FAILED(result);
      }
    }
  }
}

AZ_NODISCARD az_result
az_span_reader_read_json_pointer_token_char(az_span_reader * self, uint32_t * out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  uint32_t c;
  az_result result = az_span_reader_read_json_pointer_char(self, &c);
  if (result == AZ_ERROR_JSON_POINTER_TOKEN_END) {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  AZ_RETURN_IF_FAILED(result);
  *out = c;
  return AZ_OK;
}
