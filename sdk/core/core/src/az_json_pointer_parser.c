// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_pointer_parser.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_json_pointer_parser_get(az_span_reader * const json_pointer_parser, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(json_pointer_parser);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // read `/` if any.
  {
    az_result const result = az_span_reader_expect_char(json_pointer_parser, '/');
    if (result == AZ_ERROR_EOF) {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    AZ_RETURN_IF_FAILED(result);
  }

  size_t const begin = json_pointer_parser->i;
  while (true) {
    az_result_byte const c = az_span_reader_current(json_pointer_parser);
    switch (c) {
      case AZ_ERROR_EOF:
      case '/': {
        *out = az_span_sub(json_pointer_parser->span, begin, json_pointer_parser->i);
        return AZ_OK;
      }
      case '~': {
        az_span_reader_next(json_pointer_parser);
        az_result_byte const e = az_span_reader_current(json_pointer_parser);
        switch (e) {
          case '0':
          case '1': {
            break;
          }
          default: {
            return az_error_unexpected_char(e);
          }
        }
        break;
      }
      default: {
        AZ_RETURN_IF_FAILED(c);
        break;
      }
    }
    az_span_reader_next(json_pointer_parser);
  }
}

/** 
 * - ..-1 errors
 * - 0..255 UTF-8 byte
 * - 256 unescaped "/"
 */
typedef enum {
  AZ_JSON_POINTER_CHAR_SEPARATOR = 256,
} az_json_pointer_char;

AZ_NODISCARD AZ_INLINE az_json_pointer_char
az_span_reader_get_json_pointer_char(az_span_reader * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_result_byte const result = az_span_reader_current(self);
  switch (result) {
    case AZ_ERROR_EOF: {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    case '/': {
      return AZ_JSON_POINTER_CHAR_SEPARATOR;
    }
    case '~': {
      az_span_reader_next(self);
      az_result_byte const e = az_span_reader_current(self);
      az_span_reader_next(self);
      switch (e) {
        case '0': {
          return '~';
        }
        case '1': {
          return '/';
        }
        default: {
          return az_error_unexpected_char(e);
        }
      }
    }
    default: {
      az_span_reader_next(self);
      return result;
    }
  }
}

AZ_NODISCARD az_result az_json_pointer_token_parser_get(
    az_span_reader * const json_pointer_token_parser,
    uint8_t * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(json_pointer_token_parser);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_json_pointer_char result = az_span_reader_get_json_pointer_char(json_pointer_token_parser);
  if (result == AZ_JSON_POINTER_CHAR_SEPARATOR) {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  AZ_RETURN_IF_FAILED(result);
  *out = (uint8_t)result;
  return AZ_OK;
}
