// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

#include <az_callback.h>
#include <az_http_request.h>

#include <_az_cfg.h>

/**
 * @brief writes a header key and value to a buffer as a 0-terminated string and using a separator
 * span in between. Returns error as soon as any of the write operations fails
 *
 * @param writable_buffer
 * @param p_header
 * @param separator
 * @return az_result
 */
az_result az_write_to_buffer(
    az_span const writable_buffer,
    az_pair const p_header,
    az_const_span const separator) {
  az_span_builder writer = az_span_builder_create(writable_buffer);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, p_header.key));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, separator));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, p_header.value));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, AZ_STR_ZERO));
  return AZ_OK;
}

/**
 * @brief allocate a buffer for a header. Then reads the az_pair header and writes a buffer. Then
 * uses that buffer to set curl header. Header is set only if write operations were OK. Buffer is
 * free after setting curl header.
 *
 * @param p_header
 * @param p_headers
 * @param separator
 * @return az_result
 */
az_result az_add_header_to_curl_list(
    az_pair const p_header,
    struct curl_slist ** const p_list,
    az_const_span const separator) {
  AZ_CONTRACT_ARG_NOT_NULL(p_list);

  // allocate a buffet for header
  int16_t const buffer_size = p_header.key.size + separator.size + p_header.value.size + 1;
  uint8_t * const p_writable_buffer = (uint8_t *)malloc(buffer_size);
  if (p_writable_buffer == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  char * buffer = (char *)p_writable_buffer;

  // write buffer
  az_span const writable_buffer = (az_span){ .begin = p_writable_buffer, .size = buffer_size };
  az_result const write_result = az_write_to_buffer(writable_buffer, p_header, separator);

  // attach header only when write was OK
  if (az_succeeded(write_result)) {
    *p_list = curl_slist_append(*p_list, buffer);
  }
  // at any case, error or OK, free the allocated memory
  free(p_writable_buffer);
  return write_result;
}

/**
 * @brief loop all the headers from a HTTP request and set each header into easy curl
 *
 * @param p_hrb
 * @param p_headers
 * @return az_result
 */
az_result az_build_headers(
    az_http_request_builder const * const p_hrb,
    struct curl_slist ** p_headers) {

  az_pair header;
  for (uint16_t offset = 0; offset < p_hrb->headers_end; ++offset) {
    AZ_RETURN_IF_FAILED(az_http_request_builder_get_header(p_hrb, offset, &header));
    AZ_RETURN_IF_FAILED(
        az_add_header_to_curl_list(header, p_headers, AZ_HTTP_REQUEST_BUILDER_HEADER_SEPARATOR));
  }

  return AZ_OK;
}

/**
 * @brief writes a url request adds a cero to make it a c-string. Return error if any of the write
 * operations fails.
 *
 * @param writable_buffer
 * @param url_from_request
 * @return az_result
 */
az_result az_write_url(az_span const writable_buffer, az_const_span const url_from_request) {
  az_span_builder writer = az_span_builder_create(writable_buffer);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, url_from_request));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, AZ_STR_ZERO));
  return AZ_OK;
}

/**
 * @brief This is the function that curl will use to write response into a user provider span
 * Function receives the size of the response and must return this same number, otherwise it is
 * consider that function failed
 *
 * @param contents
 * @param size
 * @param nmemb
 * @param userp
 * @return int
 */
size_t write_to_span(void * contents, size_t size, size_t nmemb, void * userp) {
  size_t const expected_size = size * nmemb;
  size_t const size_with_extra_space = expected_size + AZ_STR_ZERO.size;
  az_span * const user_buffer = (az_span *)userp;

  // handle error when response won't feat user buffer
  if (user_buffer->size < size_with_extra_space) {
    fprintf(stderr, "response size is greater than user buffer for writing response");
    return AZ_ERROR_HTTP_FAILED_REQUEST;
  }

  // TODO: format buffer with AZ_RESPONSE_BUILDER
  memcpy(user_buffer->begin, contents, size_with_extra_space);
  // add 0 so response can be printed
  user_buffer->begin[size_with_extra_space] = *AZ_STR_ZERO.begin;

  // This callback needs to return the response size or curl will consider it as it failed
  return expected_size;
}

/**
 * handles GET request
 */
az_result az_curl_send_request(az_curl * const p_curl) {
  // send
  AZ_RETURN_IF_CURL_FAILED(curl_easy_perform(p_curl->p_curl));

  return AZ_OK;
}

/**
 * handles POST request. It handles seting up a body for request
 */
az_result az_curl_post_request(
    az_curl * const p_curl,
    az_http_request_builder const * const p_hrb) {
  // Method
  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl->p_curl, CURLOPT_POSTFIELDS, p_hrb->body.begin));

  AZ_RETURN_IF_CURL_FAILED(curl_easy_perform(p_curl->p_curl));
  return AZ_OK;
}

/**
 * @brief finds out if there are headers in the request and add them to curl header list
 *
 * @param p_curl
 * @param p_hrb
 * @return az_result
 */
az_result setup_headers(az_curl const * const p_curl, az_http_request_builder const * const p_hrb) {

  if (!az_http_request_builder_has_headers(p_hrb)) {
    // no headers, no need to set it up
    return AZ_OK;
  }

  // creates a slist for bulding curl headers
  struct curl_slist * p_list = NULL;
  // build headers into a slist as curl is expecting
  AZ_RETURN_IF_FAILED(az_build_headers(p_hrb, &p_list));
  // set all headers from slist
  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl->p_curl, CURLOPT_HTTPHEADER, p_list));

  return AZ_OK;
}

/**
 * @brief set url for the request
 *
 * @param p_curl
 * @param p_hrb
 * @return az_result
 */
az_result setup_url(az_curl const * const p_curl, az_http_request_builder const * const p_hrb) {
  // set URL as 0-terminated str
  size_t const extra_space_for_zero = AZ_STR_ZERO.size;
  size_t const url_final_size = p_hrb->url.size + extra_space_for_zero;
  // allocate buffer to add \0
  uint8_t * const p_writable_buffer = (uint8_t *)malloc(url_final_size);
  if (p_writable_buffer == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  // write url in buffer (will add \0 at the end)
  char * buffer = (char *)p_writable_buffer;
  az_span const writable_buffer = (az_span){ .begin = p_writable_buffer, .size = url_final_size };
  az_result const result = az_write_url(writable_buffer, az_span_to_const_span(p_hrb->url));
  CURLcode const set_headers_result = curl_easy_setopt(p_curl->p_curl, CURLOPT_URL, buffer);
  // free used buffer before anything else
  memset(p_writable_buffer, 0, url_final_size);
  free(buffer);

  // handle writing to buffer error
  AZ_RETURN_IF_FAILED(result);
  // handle setting curl url
  AZ_RETURN_IF_FAILED(set_headers_result);

  return AZ_OK;
}

/**
 * @brief set url the response redirection to user buffer
 *
 * @param p_curl
 * @param p_hrb
 * @return az_result
 */
az_result setup_response_redirect(az_curl const * const p_curl, az_span const * const response) {
  // check if response will be redirected to user span
  if (response != NULL) {
    AZ_RETURN_IF_CURL_FAILED(
        curl_easy_setopt(p_curl->p_curl, CURLOPT_WRITEFUNCTION, write_to_span));
    AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl->p_curl, CURLOPT_WRITEDATA, (void *)response));
  }

  return AZ_OK;
}

/**
 * @brief uses AZ_HTTP_BUILDER to set up CURL request and perform it
 *
 * @param p_hrb
 * @param response
 * @return az_result
 */
az_result az_http_client_send_request_impl(
    az_http_request_builder * const p_hrb,
    az_span const * const response) {
  az_curl p_curl;
  AZ_RETURN_IF_FAILED(az_curl_init(&p_curl));
  az_result result;

  AZ_RETURN_IF_CURL_FAILED(setup_headers(&p_curl, p_hrb));

  AZ_RETURN_IF_CURL_FAILED(setup_url(&p_curl, p_hrb));

  AZ_RETURN_IF_CURL_FAILED(setup_response_redirect(&p_curl, response));

  if (az_const_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_GET)) {
    result = az_curl_send_request(&p_curl);
  } else if (az_const_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_POST)) {
    result = az_curl_post_request(&p_curl, p_hrb);
  }

  AZ_RETURN_IF_FAILED(az_curl_done(&p_curl));
  return result;
}