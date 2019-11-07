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
    az_result result = az_span_reader_expect_char(json_pointer_parser, '/'); 
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
