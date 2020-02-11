// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_private.h"
#include "az_span_private.h"

#include <az_contract_internal.h>
#include <az_http.h>

#include <assert.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result _az_is_question_mark(az_span slice) {
  return az_span_ptr(slice)[0] == '?' ? AZ_OK : AZ_CONTINUE;
}

AZ_NODISCARD az_result az_http_request_init(
    _az_http_request * p_hrb,
    az_span method,
    az_span url,
    az_span headers_buffer,
    az_span body) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(method);
  AZ_CONTRACT_ARG_VALID_SPAN(url);
  AZ_CONTRACT_ARG_VALID_SPAN(headers_buffer);

  int32_t query_start = 0;
  az_result url_with_query = _az_scan_until(url, _az_is_question_mark, &query_start);

  *p_hrb
      = (_az_http_request){ ._internal = {
                                .method = method,
                                .url = url,
                                /* query start is set to 0 if there is not a question mark so the
                                   next time query parameter is appended, a question mark will be
                                   added at url length */
                                .query_start
                                = url_with_query == AZ_ERROR_ITEM_NOT_FOUND ? 0 : query_start,
                                .headers = headers_buffer,
                                .max_headers = az_span_capacity(headers_buffer) / sizeof(az_pair),
                                .retry_headers_start_byte_offset = 0,
                                .body = body,
                            } };

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_append_path(_az_http_request * p_hrb, az_span path) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  // get the query starting point.
  bool url_with_question_mark = p_hrb->_internal.query_start > 0;
  int32_t query_start = url_with_question_mark ? p_hrb->_internal.query_start - 1
                                               : az_span_length(p_hrb->_internal.url);

  /* use replace twice. Yes, we will have 2 right shift (one on each replace), but we rely on
   * replace functionfor doing this movements only and avoid updating manually. We could also create
   * a temp buffer to join "/" and path and then use replace. But that will cost us more stack
   * memory.
   */
  AZ_RETURN_IF_FAILED(
      az_span_replace(&p_hrb->_internal.url, query_start, query_start, AZ_SPAN_FROM_STR("/")));
  query_start += 1; // a size of "/"
  AZ_RETURN_IF_FAILED(az_span_replace(&p_hrb->_internal.url, query_start, query_start, path));
  query_start += az_span_length(path);

  // update query start
  if (url_with_question_mark) {
    p_hrb->_internal.query_start = query_start + 1;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_set_query_parameter(_az_http_request * p_hrb, az_span name, az_span value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(name);
  AZ_CONTRACT_ARG_VALID_SPAN(value);

  // name or value can't be empty
  AZ_CONTRACT(az_span_length(name) > 0 && az_span_length(value) > 0, AZ_ERROR_ARG);

  // Append either '?' or '&'
  AZ_RETURN_IF_FAILED(az_span_append(
      p_hrb->_internal.url,
      p_hrb->_internal.query_start == 0 ? AZ_SPAN_FROM_STR("?") : AZ_SPAN_FROM_STR("&"),
      &p_hrb->_internal.url));
  // update QPs starting position when it's 0
  if (p_hrb->_internal.query_start == 0) {
    p_hrb->_internal.query_start = az_span_length(p_hrb->_internal.url);
  }

  // Append parameter name
  AZ_RETURN_IF_FAILED(az_span_append(p_hrb->_internal.url, name, &p_hrb->_internal.url));

  // Append equal sym
  AZ_RETURN_IF_FAILED(
      az_span_append(p_hrb->_internal.url, AZ_SPAN_FROM_STR("="), &p_hrb->_internal.url));

  // Parameter value
  AZ_RETURN_IF_FAILED(az_span_append(p_hrb->_internal.url, value, &p_hrb->_internal.url));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_append_header(_az_http_request * p_hrb, az_span key, az_span value) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_VALID_SPAN(key);
  AZ_CONTRACT_ARG_VALID_SPAN(value);

  AZ_CONTRACT(az_span_length(key) > 0 || az_span_length(value) > 0, AZ_ERROR_ARG);

  az_span * headers_ptr = &p_hrb->_internal.headers;

  az_pair header_to_append = az_pair_init(key, value);
  return az_span_append(
      *headers_ptr,
      az_span_init((uint8_t *)&header_to_append, sizeof header_to_append, sizeof header_to_append),
      headers_ptr);
}

AZ_NODISCARD az_result
az_http_request_get_header(_az_http_request * p_hrb, int32_t index, az_pair * out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  if (index >= az_span_length(p_hrb->_internal.headers) / (int32_t)sizeof(az_pair)) {
    return AZ_ERROR_ARG;
  }

  *out_result = ((az_pair *)az_span_ptr(p_hrb->_internal.headers))[index];
  return AZ_OK;
}
