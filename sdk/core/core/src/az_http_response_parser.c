// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>

#include "az_span_private.h"
#include <az_contract_internal.h>

#include <_az_cfg.h>
#include <ctype.h>

// HTTP Response utility functions

AZ_NODISCARD AZ_INLINE az_result _az_is_char(az_span slice, uint8_t c) {
  return az_span_ptr(slice)[0] == c ? AZ_OK : AZ_CONTINUE;
}

AZ_NODISCARD az_result _az_is_a_colon(az_span slice) { return _az_is_char(slice, ':'); }
AZ_NODISCARD az_result _az_is_new_line(az_span slice) { return _az_is_char(slice, '\n'); }

AZ_NODISCARD bool _az_is_http_whitespace(uint8_t c) {
  switch (c) {
    case ' ':
    case '\t':
      return true;
      ;
    default:
      return false;
  }
}

AZ_NODISCARD az_result _az_slice_is_not_http_whitespace(az_span slice) {
  return _az_is_http_whitespace(az_span_ptr(slice)[0]) == true ? AZ_CONTINUE : AZ_OK;
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
  AZ_RETURN_IF_FAILED(az_span_slice(*self, 1, -1, self));

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
    uint64_t code = 0;
    AZ_RETURN_IF_FAILED(az_span_to_uint64(az_span_init(az_span_ptr(*self), 3, 3), &code));
    out->status_code = (az_http_status_code)code;
    // move reader
    AZ_RETURN_IF_FAILED(az_span_slice(*self, 3, -1, self));
  }

  // SP
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, space));
  // get a pointer to read response until end of reason-phrase is found
  // reason-phrase = *(HTAB / SP / VCHAR / obs-text)
  // HTAB = "\t"
  // VCHAR or obs-text is %x21-FF,
  int32_t offset = 0;
  AZ_RETURN_IF_FAILED(_az_scan_until(*self, _az_is_new_line, &offset));

  // save reason-phrase in status line now that we got the offset. Remove 1 last chars(\r)
  AZ_RETURN_IF_FAILED(az_span_slice(*self, 0, offset - 1, &out->reason_phrase));
  // move position of reader after reason-phrase (parsed done)
  AZ_RETURN_IF_FAILED(az_span_slice(*self, offset + 1, -1, self));
  // CR LF
  // AZ_RETURN_IF_FAILED(_az_is_expected_span(self, AZ_SPAN_FROM_STR("\r\n")));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_get_status_line(az_http_response * response, az_http_response_status_line * out) {
  AZ_CONTRACT_ARG_NOT_NULL(response);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // set http response to initial state if it is at any other state
  if (response->_internal.parser.next_kind != AZ_HTTP_RESPONSE_KIND_STATUS_LINE) {
    response->_internal.parser.remaining = response->_internal.http_response;
  }

  // read an HTTP status line.
  AZ_RETURN_IF_FAILED(_az_get_http_status_line(&response->_internal.parser.remaining, out));

  // set state.kind of the next HTTP response value.
  response->_internal.parser.next_kind = AZ_HTTP_RESPONSE_KIND_HEADER;

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_get_next_header(az_http_response * self, az_pair * out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);
  az_span * reader = &self->_internal.parser.remaining;
  {
    az_http_response_kind const kind = self->_internal.parser.next_kind;
    // if reader is expecting to read body (all headers were read), return ITEM_NOT_FOUND so we
    // know we reach end of headers
    if (kind == AZ_HTTP_RESPONSE_KIND_BODY) {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    // Can't read a header if status line was not previously called,
    // User needs to call az_http_response_status_line() witch would reset parser and set kind to
    // headers
    if (kind != AZ_HTTP_RESPONSE_KIND_HEADER) {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  // check if we are at the end of all headers to change state to Body.
  // We keep state to Headers if current char is not '\r' (there is another header)
  if (az_span_ptr(self->_internal.parser.remaining)[0] == '\r') {
    AZ_RETURN_IF_FAILED(_az_is_expected_span(reader, AZ_SPAN_FROM_STR("\r\n")));
    self->_internal.parser.next_kind = AZ_HTTP_RESPONSE_KIND_BODY;
    return AZ_ERROR_ITEM_NOT_FOUND;
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
    AZ_RETURN_IF_FAILED(_az_scan_until(*reader, _az_is_a_colon, &field_name_length));

    // form a header name. Reader is currently at char ':'
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, 0, field_name_length, &out->key));
    // update reader to next position after colon (add one)
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, field_name_length + 1, -1, reader));

    // OWS
    int32_t ows_len = 0;
    AZ_RETURN_IF_FAILED(_az_scan_until(*reader, _az_slice_is_not_http_whitespace, &ows_len));
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, ows_len, -1, reader));
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
    int32_t offset = 0;
    int32_t offset_value_end = offset;
    while (true) {
      uint8_t c = reader->_internal.ptr[offset];
      offset += 1;
      if (c == '\r') {
        break; // break as soon as end of value char is found
      }
      if (_az_is_http_whitespace(c)) {
        continue; // white space or tab is accepted. It can be any number after value (OWS)
      }
      if (c <= ' ') {
        return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
      }
      offset_value_end = offset; // increasing index only for valid chars,
    }
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, 0, offset_value_end, &out->value));
    // moving reader. It is currently after \r was found
    AZ_RETURN_IF_FAILED(az_span_slice(*reader, offset, -1, reader));
  }

  AZ_RETURN_IF_FAILED(_az_is_expected_span(reader, AZ_SPAN_FROM_STR("\n")));

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_get_body(az_http_response * self, az_span * out_body) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out_body);

  // Make sure get body works no matter where is the current parsing. Allow users to call get body
  // directly and ignore headers and status line
  az_http_response_kind current_parsing_section = self->_internal.parser.next_kind;
  if (current_parsing_section != AZ_HTTP_RESPONSE_KIND_BODY) {
    if (current_parsing_section == AZ_HTTP_RESPONSE_KIND_EOF) {
      // Reset parser and get status line
      az_http_response_status_line ignore = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_get_status_line(self, &ignore));
      // update current parsing section
      current_parsing_section = self->_internal.parser.next_kind;
    }
    // parse any remaining header
    if (current_parsing_section == AZ_HTTP_RESPONSE_KIND_HEADER) {
      // Parse and ignore all remaining headers
      for (az_pair h; az_http_response_get_next_header(self, &h) == AZ_OK;) {
        // ignoring header
      }
    }
  }

  // take all the remaining content from reader as body
  AZ_RETURN_IF_FAILED(az_span_slice(self->_internal.parser.remaining, 0, -1, out_body));

  self->_internal.parser.next_kind = AZ_HTTP_RESPONSE_KIND_EOF;
  return AZ_OK;
}
