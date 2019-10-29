// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_HTTP_RESPONSE_READ_H
#define AZ_HTTP_RESPONSE_READ_H

#include <az_pair.h>
#include <az_span.h>
#include <az_span_reader.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_HTTP_RESPONSE_VALUE_NONE = 0,
  AZ_HTTP_RESPONSE_VALUE_STATUS = 1,
  AZ_HTTP_RESPONSE_VALUE_HEADER = 2,
  AZ_HTTP_RESPONSE_BODY = 3,
} az_http_response_value_kind;

typedef struct {
  uint8_t major_version;
  uint8_t minor_version;
  uint16_t code;
  az_span phrase;
} az_http_response_status;

typedef struct {
  az_http_response_value_kind kind;
  union {
    az_http_response_status status;
    az_pair header;
    az_span body;
  } data;
} az_http_response_value;

typedef enum {
  AZ_HTTP_RESPONSE_STATE_STATUS = 0,
  AZ_HTTP_RESPONSE_STATE_HEADER = 1,
  AZ_HTTP_RESPONSE_STATE_BODY = 2,
  AZ_HTTP_RESPONSE_STATE_DONE = 3,
} az_http_response_state_kind;

typedef struct {
  az_span_reader reader;
  az_http_response_state_kind kind;
} az_http_response_state;

AZ_NODISCARD az_http_response_state az_http_response_state_create(az_span const buffer);

AZ_NODISCARD az_result az_http_response_state_read(
    az_http_response_state * const self,
    az_http_response_value * const out);

#include <_az_cfg_suffix.h>

#endif
