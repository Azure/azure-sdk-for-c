// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_url.h>

#include <az_http_query.h>
#include <az_span_reader.h>
#include <az_str.h>

#include <ctype.h>

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
    if (c == ':') {
      *out = az_span_take(self->span, self->i);
      az_span_reader_next(self);
      AZ_RETURN_IF_FAILED(az_span_reader_expect_span(self, AZ_STR("//")));
      return AZ_OK;
    }
    az_span_reader_next(self);
  }
}

/**
 * https://tools.ietf.org/html/rfc3986#section-3.2.3
 */
AZ_NODISCARD az_result
az_span_reader_read_url_port(az_span_reader * const self, az_span * const port) {
  {
    az_result const c = az_span_reader_current(self);
    if (c != ':') {
      *port = az_span_empty();
      return AZ_OK;
    }
    az_span_reader_next(self);
  }
  size_t const begin = self->i;
  while (true) {
    az_result_byte const c = az_span_reader_current(self);
    if (!isdigit(c)) {
      if (c == AZ_ERROR_EOF) {
        AZ_RETURN_IF_FAILED(c);
      }
      break;
    }
    az_span_reader_next(self);
  }
  *port = az_span_sub(self->span, begin, self->i);
  return AZ_OK;
}

/**
 * https://tools.ietf.org/html/rfc3986#section-3.2
 *
 * authority   = [ userinfo "@" ] host [ ":" port ]
 */
AZ_NODISCARD az_result
az_span_reader_read_url_authority(az_span_reader * const self, az_url_authority * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(out);

  bool has_userinfo = false;
  out->userinfo = az_span_empty();
  size_t begin = self->i;
  while (true) {
    az_result_byte c = az_span_reader_current(self);
    switch (c) {
      case '@': {
        if (has_userinfo) {
          return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
        }
        has_userinfo = true;
        out->userinfo = az_span_sub(self->span, begin, self->i);
        az_span_reader_next(self);
        begin = self->i;
        break;
      }
      case AZ_ERROR_EOF:
      case '?':
      case '/':
      case '#':
      case ':': {
        out->host = az_span_sub(self->span, begin, self->i);
        AZ_RETURN_IF_FAILED(az_span_reader_read_url_port(self, &out->port));
        return AZ_OK;
      }
      default: {
        AZ_RETURN_IF_FAILED(c);
        az_span_reader_next(self);
        break;
      }
    }
  }
}

/**
 * https://tools.ietf.org/html/rfc3986#section-3.3
 */
AZ_NODISCARD az_result
az_span_reader_read_url_path(az_span_reader * const self, az_span * const path) {
  size_t const begin = self->i;
  {
    az_result const c = az_span_reader_current(self);
    if (c != '/') {
      *path = az_span_empty();
      return AZ_OK;
    }
    az_span_reader_next(self);
  }
  while (true) {
    az_result_byte const c = az_span_reader_current(self);
    switch (c) {
      case AZ_ERROR_EOF:
      case '?':
      case '#': {
        *path = az_span_sub(self->span, begin, self->i);
        return AZ_OK;
      }
      default: {
        AZ_RETURN_IF_FAILED(c);
        az_span_reader_next(self);
        break;
      }
    }
  }
}

/**
 * https://tools.ietf.org/html/rfc3986#section-3.4
 */
AZ_NODISCARD az_result
az_span_reader_read_url_query(az_span_reader * const self, az_span * const query) {
  {
    az_result const c = az_span_reader_current(self);
    if (c != '?') {
      *query = az_span_empty();
      return AZ_OK;
    }
    az_span_reader_next(self);
  }
  size_t const begin = self->i;
  while (true) {
    az_result_byte const c = az_span_reader_current(self);
    switch (c) {
      case AZ_ERROR_EOF:
      case '#': {
        *query = az_span_sub(self->span, begin, self->i);
        return AZ_OK;
      }
      default: {
        AZ_RETURN_IF_FAILED(c);
        az_span_reader_next(self);
        break;
      }
    }
  }
}

/**
 * https://tools.ietf.org/html/rfc3986#section-3.5
 */
AZ_NODISCARD az_result
az_span_reader_read_url_fragment(az_span_reader * const self, az_span * const fragment) {
  {
    az_result const c = az_span_reader_current(self);
    if (c != '#') {
      *fragment = az_span_empty();
      return AZ_OK;
    }
    az_span_reader_next(self);
  }
  size_t const begin = self->i;
  while (true) {
    az_result const c = az_span_reader_current(self);
    if (c == AZ_ERROR_EOF) {
      *fragment = az_span_sub(self->span, begin, self->i);
      return AZ_OK;
    }
    AZ_RETURN_IF_FAILED(c);
    az_span_reader_next(self);
  }
}

AZ_NODISCARD az_result az_url_parse(az_span const url, az_url * const out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  az_span_reader reader = az_span_reader_create(url);

  AZ_RETURN_IF_FAILED(az_span_reader_read_url_scheme(&reader, &out->scheme));
  AZ_RETURN_IF_FAILED(az_span_reader_read_url_authority(&reader, &out->authority));
  AZ_RETURN_IF_FAILED(az_span_reader_read_url_path(&reader, &out->path));
  AZ_RETURN_IF_FAILED(az_span_reader_read_url_query(&reader, &out->query));
  AZ_RETURN_IF_FAILED(az_span_reader_read_url_fragment(&reader, &out->fragment));

  {
    az_result_byte const c = az_span_reader_current(&reader);
    if (c != AZ_ERROR_EOF) {
      AZ_RETURN_IF_FAILED(c);
      return az_error_unexpected_char(c);
    }
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_host_read_domain(az_span * const host, az_span * const domain) {
  AZ_CONTRACT_ARG_NOT_NULL(host);
  AZ_CONTRACT_ARG_NOT_NULL(domain);

  size_t const size = host->size;
  if (size == 0) {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  size_t i = size - 1;
  do {
    az_result_byte const c = az_span_get(*host, i);
    if (c == '.') {
      *domain = az_span_sub(*host, i + 1, size);
      host->size = i;
      return AZ_OK;
    }
    --i;
  } while (0 < i);
  *domain = *host;
  *host = az_span_empty();
  return AZ_OK;
}
