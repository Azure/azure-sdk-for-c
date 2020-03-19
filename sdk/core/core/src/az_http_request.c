// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_http_private.h"
#include "az_span_private.h"

#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <assert.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result _az_is_question_mark(az_span slice)
{
  return az_span_ptr(slice)[0] == '?' ? AZ_OK : AZ_CONTINUE;
}

AZ_NODISCARD az_result az_http_request_init(
    _az_http_request* p_request,
    az_context* context,
    az_span method,
    az_span url,
    az_span headers_buffer,
    az_span body)
{
  AZ_PRECONDITION_NOT_NULL(p_request);
  AZ_PRECONDITION_VALID_SPAN(method, 1, false);
  AZ_PRECONDITION_VALID_SPAN(url, 1, false);
  AZ_PRECONDITION_VALID_SPAN(headers_buffer, 0, false);

  int32_t query_start = 0;
  az_result url_with_query = _az_scan_until(url, _az_is_question_mark, &query_start);

  *p_request
      = (_az_http_request){ ._internal = {
                                .context = context,
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

AZ_NODISCARD az_result az_http_request_append_path(_az_http_request* p_request, az_span path)
{
  AZ_PRECONDITION_NOT_NULL(p_request);

  // get the query starting point.
  bool url_with_question_mark = p_request->_internal.query_start > 0;
  int32_t query_start = url_with_question_mark ? p_request->_internal.query_start - 1
                                               : az_span_length(p_request->_internal.url);

  /* use replace twice. Yes, we will have 2 right shift (one on each replace), but we rely on
   * replace functionfor doing this movements only and avoid updating manually. We could also create
   * a temp buffer to join "/" and path and then use replace. But that will cost us more stack
   * memory.
   */
  AZ_RETURN_IF_FAILED(
      _az_span_replace(&p_request->_internal.url, query_start, query_start, AZ_SPAN_FROM_STR("/")));
  query_start += 1; // a size of "/"
  AZ_RETURN_IF_FAILED(_az_span_replace(&p_request->_internal.url, query_start, query_start, path));
  query_start += az_span_length(path);

  // update query start
  if (url_with_question_mark)
  {
    p_request->_internal.query_start = query_start + 1;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_set_query_parameter(_az_http_request* p_request, az_span name, az_span value)
{
  AZ_PRECONDITION_NOT_NULL(p_request);
  AZ_PRECONDITION_VALID_SPAN(name, 1, false);
  AZ_PRECONDITION_VALID_SPAN(value, 1, false);

  // name or value can't be empty
  AZ_PRECONDITION(az_span_length(name) > 0 && az_span_length(value) > 0);

  // Append either '?' or '&'
  AZ_RETURN_IF_FAILED(az_span_append(
      p_request->_internal.url,
      p_request->_internal.query_start == 0 ? AZ_SPAN_FROM_STR("?") : AZ_SPAN_FROM_STR("&"),
      &p_request->_internal.url));
  // update QPs starting position when it's 0
  if (p_request->_internal.query_start == 0)
  {
    p_request->_internal.query_start = az_span_length(p_request->_internal.url);
  }

  // Append parameter name
  AZ_RETURN_IF_FAILED(az_span_append(p_request->_internal.url, name, &p_request->_internal.url));

  // Append equal sym
  AZ_RETURN_IF_FAILED(
      az_span_append(p_request->_internal.url, AZ_SPAN_FROM_STR("="), &p_request->_internal.url));

  // Parameter value
  AZ_RETURN_IF_FAILED(az_span_append(p_request->_internal.url, value, &p_request->_internal.url));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_request_append_header(_az_http_request* p_request, az_span key, az_span value)
{
  AZ_PRECONDITION_NOT_NULL(p_request);
  AZ_PRECONDITION_VALID_SPAN(key, 1, false);
  AZ_PRECONDITION_VALID_SPAN(value, 1, false);

  AZ_PRECONDITION(az_span_length(key) > 0 || az_span_length(value) > 0);

  az_span* headers_ptr = &p_request->_internal.headers;

  az_pair header_to_append = az_pair_init(key, value);
  return az_span_append(
      *headers_ptr,
      az_span_init((uint8_t*)&header_to_append, sizeof header_to_append, sizeof header_to_append),
      headers_ptr);
}

AZ_NODISCARD az_result
az_http_request_get_header(_az_http_request const* request, int32_t index, az_pair* out_header)
{
  AZ_PRECONDITION_NOT_NULL(request);
  AZ_PRECONDITION_NOT_NULL(out_header);

  if (index >= _az_http_request_headers_count(request))
  {
    return AZ_ERROR_ARG;
  }

  *out_header = ((az_pair*)az_span_ptr(request->_internal.headers))[index];
  return AZ_OK;
}

AZ_NODISCARD az_result az_http_request_get_parts(
    _az_http_request const* request,
    az_http_method* out_method,
    az_span* out_url,
    az_span* out_body)
{
  *out_method = request->_internal.method;
  *out_url = request->_internal.url;
  *out_body = request->_internal.body;
  return AZ_OK;
}
