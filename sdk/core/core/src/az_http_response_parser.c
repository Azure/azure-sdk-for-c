// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>

#include <az_http_result.h>
#include <az_str.h>

#include <_az_cfg.h>

// HTTP Response utility functions

AZ_NODISCARD AZ_INLINE bool az_is_reason_phrase_symbol(az_result_byte const c) {
  return c == '\t' || c >= ' ';
}

AZ_NODISCARD bool az_is_http_whitespace(az_result_byte const c) {
  switch (c) {
    case ' ':
    case '\t':
      return true;
    default:
      return false;
  }
}

// Reading HTTP Response parts from @az_span_reader.

/**
 * Status line https://tools.ietf.org/html/rfc7230#section-3.1.2
 * HTTP-version SP status-code SP reason-phrase CRLF
 */
AZ_NODISCARD az_result az_span_reader_get_http_status_line(
    az_span_reader * const self,
    az_http_response_status_line * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
  // https://tools.ietf.org/html/rfc7230#section-2.6
  AZ_RETURN_IF_FAILED(az_span_reader_expect_span(self, AZ_STR("HTTP/")));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_digit(self, &out->major_version));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(self, '.'));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_digit(self, &out->minor_version));

  // SP = " "
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(self, ' '));

  // status-code = 3DIGIT
  {
    uint16_t status_code = 0;
    for (int i = 3; i > 0;) {
      --i;
      uint8_t digit = 0;
      AZ_RETURN_IF_FAILED(az_span_reader_expect_digit(self, &digit));
      status_code = status_code * 10 + digit;
    }
    out->status_code = status_code;
  }

  // SP
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(self, ' '));

  size_t const begin = self->i;

  // reason-phrase = *(HTAB / SP / VCHAR / obs-text)
  // HTAB = "\t"
  // VCHAR or obs-text is %x21-FF,
  while (az_is_reason_phrase_symbol(az_span_reader_current(self))) {
    az_span_reader_next(self);
  }

  out->reason_phrase = az_span_sub(self->span, begin, self->i);
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_span_reader_get_http_value_kind(az_span_reader * const self, az_http_response_kind * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_result_byte const c = az_span_reader_current(self);
  if (c == AZ_CR) {
    az_span_reader_next(self);
    AZ_RETURN_IF_FAILED(az_span_reader_expect_char(self, AZ_LF));
    *out = AZ_HTTP_RESPONSE_BODY;
  } else {
    *out = AZ_HTTP_RESPONSE_HEADER;
  }
  return AZ_OK;
}

// An HTTP parser.
//
// It accesses a response buffer only by `az_span_reader_current()`
// and move to the next position using `az_span_reader_next()`.
//
// Note: no other functions are used to access the response buffer.

AZ_NODISCARD az_result
az_http_response_parser_init(az_http_response_parser * const out, az_span const buffer) {
  *out = (az_http_response_parser){
    .reader = az_span_reader_create(buffer),
    .kind = AZ_HTTP_RESPONSE_STATUS_LINE,
  };
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_http_response_parser_set_kind(az_http_response_parser * const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);

  az_span_reader * const p_reader = &self->reader;
  az_result_byte const c = az_span_reader_current(p_reader);
  if (c == AZ_CR) {
    az_span_reader_next(p_reader);
    AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, AZ_LF));
    self->kind = AZ_HTTP_RESPONSE_BODY;
  } else {
    self->kind = AZ_HTTP_RESPONSE_HEADER;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_parser_get_status_line(
    az_http_response_parser * const self,
    az_http_response_status_line * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // check the status.
  if (self->kind != AZ_HTTP_RESPONSE_STATUS_LINE) {
    return AZ_HTTP_ERROR_INVALID_STATE;
  }

  az_span_reader * const p_reader = &self->reader;

  // read an HTTP status line.
  AZ_RETURN_IF_FAILED(az_span_reader_get_http_status_line(p_reader, out));

  // CR LF
  AZ_RETURN_IF_FAILED(az_span_reader_expect_span(p_reader, AZ_STR(AZ_CRLF)));

  // set state.kind of the next HTTP response value.
  AZ_RETURN_IF_FAILED(az_http_response_parser_set_kind(self));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_parser_get_next_header(az_http_response_parser * const self, az_pair * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  {
    az_http_response_kind const kind = self->kind;
    if (kind == AZ_HTTP_RESPONSE_BODY) {
      return AZ_HTTP_ERROR_NO_MORE_HEADERS;
    }
    if (kind != AZ_HTTP_RESPONSE_HEADER) {
      return AZ_HTTP_ERROR_INVALID_STATE;
    }
  }

  az_span_reader * const p_reader = &self->reader;

  // https://tools.ietf.org/html/rfc7230#section-3.2

  // header-field   = field-name ":" OWS field-value OWS

  // field-name     = token
  {
    size_t const field_name_begin = p_reader->i;

    // https://tools.ietf.org/html/rfc7230#section-3.2.6
    // token = 1*tchar
    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / "^" /
    //         "_" / "`" / "|" / "~" / DIGIT / ALPHA;
    // any VCHAR,
    //    except delimiters
    while (true) {
      az_result_byte const c = az_span_reader_current(p_reader);
      az_span_reader_next(p_reader);
      if (c == ':') {
        break;
      }
      // c must be VCHAR.
      if (c <= ' ') {
        return az_error_unexpected_char(c);
      }
    }

    // form a header name. In the current position, p_reader->i points on the next character after
    // `:` so we subtract 1.
    out->key = az_span_sub(p_reader->span, field_name_begin, p_reader->i - 1);
  }

  // OWS
  while (true) {
    az_result_byte const c = az_span_reader_current(p_reader);
    if (!az_is_http_whitespace(c)) {
      break;
    }
    az_span_reader_next(p_reader);
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
    size_t const field_value_begin = p_reader->i;
    size_t field_value_end = field_value_begin;
    while (true) {
      az_result_byte const c = az_span_reader_current(p_reader);
      az_span_reader_next(p_reader);
      if (c == AZ_CR) {
        break;
      }
      if (az_is_http_whitespace(c)) {
        continue;
      }
      if (c <= ' ') {
        return az_error_unexpected_char(c);
      }
      field_value_end = p_reader->i;
    }
    out->value = az_span_sub(p_reader->span, field_value_begin, field_value_end);
  }

  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, AZ_LF));

  // set state.kind of the next HTTP response value.
  AZ_RETURN_IF_FAILED(az_http_response_parser_set_kind(self));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_parser_get_body(az_http_response_parser * const self, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  if (self->kind != AZ_HTTP_RESPONSE_BODY) {
    return AZ_HTTP_ERROR_INVALID_STATE;
  }

  az_span_reader * const p_reader = &self->reader;
  *out = az_span_drop(p_reader->span, p_reader->i);
  self->kind = AZ_HTTP_RESPONSE_NONE;
  return AZ_OK;
}

// HTTP Response get functions

AZ_NODISCARD az_result
az_http_response_get_status_line(az_span const self, az_http_response_status_line * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_span_reader reader = az_span_reader_create(self);
  return az_span_reader_get_http_status_line(&reader, out);
}

AZ_NODISCARD az_result az_http_response_parser_init_from_header(
    az_http_response_parser * const out,
    az_span const response,
    az_pair const * const p_header) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  AZ_RETURN_IF_FAILED(az_http_response_parser_init(out, response));

  az_span_reader * const p_reader = &out->reader;
  az_span const value = p_header->value;
  if (value.size == 0 || value.begin == NULL) {
    // read a status line and ignore it
    az_http_response_status_line status_line;
    AZ_RETURN_IF_FAILED(az_span_reader_get_http_status_line(p_reader, &status_line));
  } else {
    // change a reader position.
    size_t const i = value.begin + value.size - p_reader->span.begin;
    AZ_RETURN_IF_FAILED(az_span_reader_set_pos(p_reader, i));
  }

  // read CRLF.
  AZ_RETURN_IF_FAILED(az_span_reader_expect_span(p_reader, AZ_STR(AZ_CRLF)));

  // set kind of the next HTTP response value.
  AZ_RETURN_IF_FAILED(az_http_response_parser_set_kind(out));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_get_next_header(az_span const self, az_pair * const p_header) {
  AZ_CONTRACT_ARG_NOT_NULL(p_header);

  az_http_response_parser parser;
  AZ_RETURN_IF_FAILED(az_http_response_parser_init_from_header(&parser, self, p_header));
  AZ_RETURN_IF_FAILED(az_http_response_parser_get_next_header(&parser, p_header));
  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_get_body(az_span const self, az_pair * const p_last_header, az_span * const body) {
  AZ_CONTRACT_ARG_NOT_NULL(p_last_header);
  AZ_CONTRACT_ARG_NOT_NULL(body);

  az_http_response_parser parser;
  AZ_RETURN_IF_FAILED(az_http_response_parser_init_from_header(&parser, self, p_last_header));
  AZ_RETURN_IF_FAILED(az_http_response_parser_get_body(&parser, body));
  return AZ_OK;
}

/**
 * Get an HTTP header by name.
 */
AZ_NODISCARD az_result az_http_response_get_header(
    az_span const self,
    az_span const header_name,
    az_span * const header_value) {
  AZ_CONTRACT_ARG_NOT_NULL(header_value);

  az_http_response_parser parser;
  AZ_RETURN_IF_FAILED(az_http_response_parser_init(&parser, self));

  { 
    az_http_response_status_line status_line;
    AZ_RETURN_IF_FAILED(az_http_response_parser_get_status_line(&parser, &status_line)); 
  }

  while (true) {
    az_pair header;
    AZ_RETURN_IF_FAILED(az_http_response_parser_get_next_header(&parser, &header));
    if (az_span_eq_ascii_ignore_case(header_name, header.key)) {
      *header_value = header.value;
      return AZ_OK;
    }
  }
}
