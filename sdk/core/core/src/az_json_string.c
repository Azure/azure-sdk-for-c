// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_string.h>

#include <az_hex.h>
#include <az_str.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE az_result_byte az_hex_to_digit(az_result_byte const c) {
  if (isdigit(c)) {
    return c - '0';
  }
  if ('a' <= c && c <= 'f') {
    return c - AZ_HEX_LOWER_OFFSET;
  }
  if ('A' <= c && c <= 'F') {
    return c - AZ_HEX_UPPER_OFFSET;
  }
  return az_error_unexpected_char(c);
}

AZ_NODISCARD AZ_INLINE az_result_byte az_json_esc_decode(az_result_byte const c) {
  switch (c) {
    case '\\':
    case '"':
    case '/': {
      return c;
    }
    case 'b': {
      return '\b';
    }
    case 'f': {
      return '\f';
    }
    case 'n': {
      return '\n';
    }
    case 'r': {
      return '\r';
    }
    case 't': {
      return '\t';
    }
    default:
      return az_error_unexpected_char(c);
  }
}

AZ_NODISCARD az_span az_json_esc_encode(az_result_byte const c) {
  switch (c) {
    case '\\': {
      return AZ_STR("\\\\");
    }
    case '"': {
      return AZ_STR("\\\"");
    }
    case '\b': {
      return AZ_STR("\\b");
    }
    case '\f': {
      return AZ_STR("\\f");
    }
    case '\n': {
      return AZ_STR("\\n");
    }
    case '\r': {
      return AZ_STR("\\r");
    }
    case '\t': {
      return AZ_STR("\\t");
    }
    default: {
      return (az_span){ 0 };
    }
  }
}

AZ_NODISCARD az_result
az_span_reader_read_json_string_char(az_span_reader * const self, uint32_t * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_result_byte const result = az_span_reader_current(self);
  switch (result) {
    case AZ_ERROR_EOF: {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    case '"': {
      return AZ_ERROR_JSON_STRING_END;
    }
    case '\\': {
      az_span_reader_next(self);
      az_result const c = az_span_reader_current(self);
      az_span_reader_next(self);
      if (c == 'u') {
        uint32_t r = 0;
        for (size_t i = 0; i < 4; ++i, az_span_reader_next(self)) {
          az_result_byte const digit = az_hex_to_digit(az_span_reader_current(self));
          AZ_RETURN_IF_FAILED(digit);
          r = (r << 4) + digit;
        }
        *out = r;
      } else {
        az_result_byte const r = az_json_esc_decode(c);
        AZ_RETURN_IF_FAILED(r);
        *out = r;
      }
      return AZ_OK;
    }
    default: {
      if (result < 0x20) {
        return az_error_unexpected_char(result);
      }
      az_span_reader_next(self);
      *out = (uint16_t)result;
      return AZ_OK;
    }
  }
}
