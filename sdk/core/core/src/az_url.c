// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_url.h>

#include <az_http_query.h>
#include <az_span_reader.h>
#include <az_str.h>

#include <_az_cfg.h>

/**
 * https://tools.ietf.org/html/rfc3986#section-3.1
 */
AZ_NODISCARD az_result
az_span_reader_read_url_scheme(az_span_reader * const self, az_span * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  while (true) {
    az_result_byte const c = az_span_reader_current(self);
    AZ_RETURN_IF_FAILED(c);
    az_span_reader_next(self);
    if (c == ':') {
      *out = az_span_take(self->span, self->i);
      AZ_RETURN_IF_FAILED(az_span_reader_expect_span(self, AZ_STR("//")));
      return AZ_OK;
    }
  }
}

/**
 * https://tools.ietf.org/html/rfc3986#section-3.2
 */
AZ_NODISCARD az_result
az_span_reader_read_url_authority(az_span_reader * const self, az_url_authority * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  size_t begin = self->i;
  while (true) {
    az_result_byte const c = az_span_reader_current(self);
    switch (c) {
      case AZ_ERROR_EOF: {
      }
      
    }
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_url_parse(az_span const url, az_url * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_span_reader reader = az_span_reader_create(url);

  AZ_RETURN_IF_FAILED(az_span_reader_read_url_scheme(&reader, &out->scheme));

  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_host_read_domain(az_span * const host, az_span * const domain) {
  AZ_CONTRACT_ARG_NOT_NULL(host);
  AZ_CONTRACT_ARG_NOT_NULL(domain);

  return AZ_ERROR_NOT_IMPLEMENTED;
}
