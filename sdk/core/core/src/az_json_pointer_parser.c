// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_json_pointer_parser.h>

#include <_az_cfg.h>

AZ_NODISCARD static az_result az_span_reader_get_json_pointer_char(
    az_span_reader * const self,
    uint8_t * const out) {
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

AZ_NODISCARD az_result az_span_reader_read_json_pointer_token(
    az_span_reader * const json_pointer_parser,
    az_span * const out) {
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
    uint8_t c = { 0 };
    az_result const result = az_span_reader_get_json_pointer_char(json_pointer_parser, &c);
    switch (result) {
      case AZ_ERROR_ITEM_NOT_FOUND:
      case AZ_ERROR_JSON_POINTER_TOKEN_END: {
        *out = az_span_sub(json_pointer_parser->span, begin, json_pointer_parser->i);
        return AZ_OK;
      }
    }
    AZ_RETURN_IF_FAILED(result);
  }
}

AZ_NODISCARD az_result az_span_reader_read_json_pointer_token_char(
    az_span_reader * const json_pointer_token_parser,
    uint8_t * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(json_pointer_token_parser);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  uint8_t c;
  az_result const result = az_span_reader_get_json_pointer_char(json_pointer_token_parser, &c);
  if (result == AZ_ERROR_JSON_POINTER_TOKEN_END) {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  AZ_RETURN_IF_FAILED(result);
  *out = (uint8_t)c;
  return AZ_OK;
}
