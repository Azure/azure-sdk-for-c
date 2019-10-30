// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_parser.h>

#include <az_http_result.h>
#include <az_str.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result
az_http_response_parser_init(az_span const buffer, az_http_response_parser * const out) {
  *out = (az_http_response_parser){
    .reader = az_span_reader_create(buffer),
    .kind = AZ_HTTP_RESPONSE_STATUS_LINE,
  };
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE bool az_is_reason_phrase_symbol(az_result_byte const c) {
  return c == '\t' || c >= ' ';
}

AZ_NODISCARD AZ_INLINE az_result
az_http_response_parser_set_kind(az_http_response_parser * const self) {
  az_span_reader * const p_reader = &self->reader;

  // check if it's an empty line.
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

/**
 * Status line https://tools.ietf.org/html/rfc7230#section-3.1.2
 * HTTP-version SP status-code SP reason-phrase CRLF
 */
AZ_NODISCARD az_result az_http_response_parser_read_status_line(
    az_http_response_parser * const self,
    az_http_response_status_line * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  if (self->kind != AZ_HTTP_RESPONSE_HEADER) {
    return AZ_HTTP_ERROR_INVALID_STATE;
  }

  az_span_reader * const p_reader = &self->reader;

  // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
  // https://tools.ietf.org/html/rfc7230#section-2.6
  AZ_RETURN_IF_FAILED(az_span_reader_expect_span(p_reader, AZ_STR("HTTP/")));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_digit(p_reader, &out->major_version));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, '.'));
  AZ_RETURN_IF_FAILED(az_span_reader_expect_digit(p_reader, &out->minor_version));

  // SP = " "
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, ' '));

  // status-code = 3DIGIT
  {
    uint16_t status_code = 0;
    for (int i = 3; i > 0;) {
      --i;
      uint8_t digit = 0;
      AZ_RETURN_IF_FAILED(az_span_reader_expect_digit(p_reader, &digit));
      status_code = status_code * 10 + digit;
    }
    out->status_code = status_code;
  }

  // SP
  AZ_RETURN_IF_FAILED(az_span_reader_expect_char(p_reader, ' '));

  size_t const begin = p_reader->i;

  // reason-phrase = *(HTAB / SP / VCHAR / obs-text)
  // HTAB = "\t"
  // VCHAR or obs-text is %x21-FF,
  while (az_is_reason_phrase_symbol(az_span_reader_current(p_reader))) {
    az_span_reader_next(p_reader);
  }

  out->reason_phrase = az_span_sub(p_reader->span, begin, p_reader->i);

  // CR LF
  AZ_RETURN_IF_FAILED(az_span_reader_expect_span(p_reader, AZ_STR(AZ_CRLF)));

  // set state.kind of the next HTTP response value.
  AZ_RETURN_IF_FAILED(az_http_response_parser_set_kind(self));

  return AZ_OK;
}

AZ_NODISCARD bool az_is_http_whitespace(az_result_byte const c) {
  switch (c) {
    case ' ':
    case '\t':
      return true;
  }
  return false;
}

/**
 * https://tools.ietf.org/html/rfc7230#section-3.2
 */
AZ_NODISCARD az_result az_http_response_parser_read_header(
    az_http_response_parser * const self,
    az_http_response_header * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  {
    az_result const kind = self->kind;
    if (kind == AZ_HTTP_RESPONSE_BODY) {
      return AZ_HTTP_ERROR_NO_MORE_HEADERS;
    }
    if (kind != AZ_HTTP_RESPONSE_HEADER) {
      return AZ_HTTP_ERROR_INVALID_STATE;
    }
  }

  az_span_reader * const p_reader = &self->reader;

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

AZ_NODISCARD az_result az_http_response_parser_read_body(
    az_http_response_parser * const self,
    az_http_response_body * const out) {
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
