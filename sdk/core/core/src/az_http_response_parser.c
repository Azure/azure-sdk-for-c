// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>

#include "az_span_private.h"
#include "az_span_reader_private.h"

#include "az_str_private.h"

#include <_az_cfg.h>
#include <ctype.h>

// HTTP Response utility functions

typedef bool (*az_read_condition)(uint8_t c);

// PRIVATE. read until condition is true on character.
// Then return number of positions read with output parameter
AZ_NODISCARD az_result _az_read_while(
    az_span * self,
    az_read_condition condition,
    bool asci_only,
    int32_t * out_read_count) {
  for (uint8_t * ptr = az_span_ptr(*self); condition(*ptr); ptr++) {
    *out_read_count += 1;
    if (asci_only && *ptr <= ' ') {
      return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
    }
  }
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE bool _az_is_reason_phrase_symbol(uint8_t c) { return c == '\t' || c >= ' '; }

AZ_NODISCARD AZ_INLINE bool _az_is_not_a_colon(uint8_t c) { return c != ':'; }

AZ_NODISCARD AZ_INLINE bool _az_is_not_a_cr(uint8_t c) { return c != AZ_CR; }

AZ_NODISCARD bool _az_is_http_whitespace(uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
      return true;
    default:
      return false;
  }
}

AZ_NODISCARD az_result _az_is_expected_span(az_span * self, az_span expected) {
  az_span actual_span = { 0 };

  int32_t expected_length = az_span_length(expected);
  AZ_RETURN_IF_FAILED(az_span_slice(*self, 0, expected_length, &actual_span));

  if (!az_span_is_equal(actual_span, expected)) {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  // move reader after the expected span (means it was parsed as expected)
  AZ_RETURN_IF_FAILED(az_span_slice(*self, expected_length, -1, self));

  return AZ_OK;
}

/* PRIVATE Function. parse next  */
AZ_NODISCARD az_result _az_get_digit(az_span * self, uint8_t * save_here) {

  uint8_t c_ptr = az_span_ptr(*self)[0];
  if (!isdigit(c_ptr)) {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  //
  *save_here = (uint8_t)(c_ptr - '0');

  // move reader after the expected digit (means it was parsed as expected)
  AZ_RETURN_IF_FAILED(az_span_slice(*self, 0, 1, self));

  return AZ_OK;
}

/**
 * Status line https://tools.ietf.org/html/rfc7230#section-3.1.2
 * HTTP-version SP status-code SP reason-phrase CRLF
 */
AZ_NODISCARD az_result
_az_get_http_status_line(az_span * const self, az_http_response_status_line * const out) {

  // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
  // https://tools.ietf.org/html/rfc7230#section-2.6
  az_span const start = AZ_SPAN_FROM_STR("HTTP/");
  az_span const dot = AZ_SPAN_FROM_STR(".");
  az_span const space = AZ_SPAN_FROM_STR(" ");

  // parse and move reader if success
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, start));
  AZ_RETURN_IF_FAILED(_az_get_digit(self, &out->major_version));
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, dot));
  AZ_RETURN_IF_FAILED(_az_get_digit(self, &out->minor_version));

  // SP = " "
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, space));

  // status-code = 3DIGIT
  {
    uint16_t status_code = 0;
    for (int i = 0; i < 3; ++i) {
      uint8_t digit = 0;
      AZ_RETURN_IF_FAILED(_az_get_digit(self, &digit));
      status_code = status_code * 10 + digit;
    }
    out->status_code = status_code;
  }

  // SP
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, space));
  // get a pointer to read response until end of reason-phrase is found
  // reason-phrase = *(HTAB / SP / VCHAR / obs-text)
  // HTAB = "\t"
  // VCHAR or obs-text is %x21-FF,
  int32_t offset = 0;
  AZ_RETURN_IF_FAILED(_az_read_while(self, _az_is_reason_phrase_symbol, false, &offset));

  // save reason-phrase in status line now that we got the offset
  AZ_RETURN_IF_FAILED(az_span_slice(*self, 0, offset, &out->reason_phrase));
  // move position of reader after reason-phrase (parsed done)
  AZ_RETURN_IF_FAILED(az_span_slice(*self, offset, -1, &out->reason_phrase));

  // CR LF
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, AZ_SPAN_FROM_STR(AZ_CRLF)));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_get_status_line(az_http_response * response, az_http_response_status_line * out) {
  AZ_CONTRACT_ARG_NOT_NULL(response);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // set http response to initial state
  response->_internal.reader = response->_internal.http_response;

  // parse status line
  response->_internal.kind = AZ_HTTP_RESPONSE_KIND_STATUS_LINE;

  // read an HTTP status line.
  AZ_RETURN_IF_FAILED(_az_get_http_status_line(&response->_internal.reader, out));

  // set state.kind of the next HTTP response value.
  response->_internal.kind = AZ_HTTP_RESPONSE_KIND_HEADER;

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_get_next_header(az_http_response * self, az_pair * out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  az_span * reader = &self->_internal.reader;
  {
    az_http_response_kind const kind = self->_internal.kind;
    // if reader is expecting to read body (all headers were read), return ITEM_NOT_FOUND so we know
    // we reach end of headers
    if (kind == AZ_HTTP_RESPONSE_KIND_BODY) {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    if (kind != AZ_HTTP_RESPONSE_KIND_HEADER) {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  // https://tools.ietf.org/html/rfc7230#section-3.2
  // header-field   = field-name ":" OWS field-value OWS
  // field-name     = token
  {
    int32_t field_name_length = 0;
    // https://tools.ietf.org/html/rfc7230#section-3.2.6
    // token = 1*tchar
    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / "^" /
    //         "_" / "`" / "|" / "~" / DIGIT / ALPHA;
    // any VCHAR,
    //    except delimiters
    AZ_RETURN_IF_FAILED(_az_read_while(reader, _az_is_not_a_colon, true, &field_name_length));

    // form a header name. Reader is currently at char ':'
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, 0, field_name_length, &out->key));
    // update reader to next position after colon (add one)
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, field_name_length + 1, -1, reader));

    // OWS
    int32_t ows_len = 0;
    AZ_RETURN_IF_FAILED(_az_read_while(reader, _az_is_http_whitespace, false, &ows_len));
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, 0, ows_len, reader));
  }
  // field-value    = *( field-content / obs-fold )
  // field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
  // field-vchar    = VCHAR / obs-text
  //
  // obs-fold       = CRLF 1*( SP / HTAB )
  //                ; obsolete line folding
  //                ; see Section 3.2.4
  //
  // Note: obs-fold is not implemented.
  {
    int32_t value_length = 0;
    AZ_RETURN_IF_FAILED(_az_read_while(reader, _az_is_not_a_cr, true, &value_length));

    AZ_RETURN_IF_FAILED(az_span_slice(*reader, 0, value_length, &out->value));
    // moving reader one extra position to jump '\r'
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, value_length + 1, -1, reader));
  }
  AZ_RETURN_IF_FAILED(_az_is_expected_span(reader, AZ_SPAN_FROM_STR("\n")));

  // check if we are at the end of all headers to change state to Body.
  // We keep maintain state to Headers if current char is not '\r' (there is another header)
  if (az_span_ptr(self->_internal.reader)[0] == AZ_CR) {
    AZ_RETURN_IF_FAILED(_az_is_expected_span(reader, AZ_SPAN_FROM_STR("\n")));
    self->_internal.kind = AZ_HTTP_RESPONSE_KIND_BODY;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_get_body(az_http_response * self, az_span * out_body) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out_body);

  // Make sure get body works no matter where is the current parsing. Allow users to call get body
  // directly and ignore headers and status line
  az_http_response_kind current_parsing_section = self->_internal.kind;
  if (current_parsing_section != AZ_HTTP_RESPONSE_KIND_BODY) {
    if (current_parsing_section == AZ_HTTP_RESPONSE_KIND_NONE) {
      // Parse and ignore status line
      az_http_response_status_line ignore = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_get_status_line(self, &ignore));
    }
    if (current_parsing_section == AZ_HTTP_RESPONSE_KIND_HEADER) {
      // Parse and ignore all remaining headers
      for (az_pair h; az_http_response_get_next_header(self, &h) == AZ_OK;) {
        // ignoring header
      }
    }
  }

  // take all the remaining content from reader as body
  AZ_RETURN_IF_FAILED(az_span_slice(self->_internal.reader, 0, -1, out_body));

  self->_internal.kind = AZ_HTTP_RESPONSE_KIND_NONE;
  return AZ_OK;
}
