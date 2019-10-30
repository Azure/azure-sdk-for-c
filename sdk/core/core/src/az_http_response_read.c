// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_read.h>

#include <az_http_result.h>
#include <az_str.h>

#include <_az_cfg.h>

AZ_NODISCARD az_http_response_state az_http_response_state_create(az_span const buffer) {
  return (az_http_response_state){
    .reader = az_span_reader_create(buffer),
    .kind = AZ_HTTP_RESPONSE_STATUS_LINE,
  };
}

AZ_NODISCARD AZ_INLINE bool az_is_reason_phrase_symbol(az_result_byte const c) {
  return c == '\t' || c >= ' ';
}

AZ_NODISCARD AZ_INLINE az_result
az_http_response_state_set_kind(az_http_response_state * const self) {
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
AZ_NODISCARD az_result az_http_response_state_read_status(
    az_http_response_state * const self,
    az_http_response_status_line * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

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
  AZ_RETURN_IF_FAILED(az_http_response_state_set_kind(self));

  return AZ_OK;
}

/**
 * https://tools.ietf.org/html/rfc7230#section-3.2
 */
AZ_NODISCARD az_result az_http_response_state_read_header(
    az_http_response_state * const self,
    az_http_response_header * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  // az_span_reader * const p_reader = &self->reader;

  // set state.kind of the next HTTP response value.
  AZ_RETURN_IF_FAILED(az_http_response_state_set_kind(self));

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_state_read_body(
    az_http_response_state * const self,
    az_http_response_body * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_span_reader * const p_reader = &self->reader;
  *out = az_span_drop(p_reader->span, p_reader->i);
  self->kind = AZ_HTTP_RESPONSE_NONE;
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_state_read(
    az_http_response_state * const self,
    az_http_response_value * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  switch (self->kind) {
    case AZ_HTTP_RESPONSE_STATUS_LINE: {
      az_http_response_status_line status_line = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_state_read_status(self, &status_line));
      *out = (az_http_response_value){
        .kind = AZ_HTTP_RESPONSE_STATUS_LINE,
        .data.status_line = status_line,
      };
      return AZ_OK;
    }
    case AZ_HTTP_RESPONSE_HEADER: {
      az_http_response_header header = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_state_read_header(self, &header));
      *out = (az_http_response_value){
        .kind = AZ_HTTP_RESPONSE_HEADER,
        .data.header = header,
      };
      return AZ_OK;
    }
    case AZ_HTTP_RESPONSE_BODY: {
      az_http_response_body body = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_state_read_body(self, &body));
      *out = (az_http_response_value){
        .kind = AZ_HTTP_RESPONSE_BODY,
        .data.body = body,
      };
      return AZ_OK;
    }

    default:
      return AZ_HTTP_ERROR_INVALID_STATE;
  }
}
