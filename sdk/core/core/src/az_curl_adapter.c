// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

#include <az_http_request.h>

#include <_az_cfg.h>

az_span const AZ_HTTP_REQUEST_BUILDER_HEADER_SEPARATOR = AZ_CONST_STR(": ");
az_span const AZ_CURL_ADAPTER_RESPONSE_PLACEHOLDER = AZ_CONST_STR("HTTP/2.0 205 \r\n");

AZ_NODISCARD char az_digit_to_char(size_t const digit) {
  switch (digit) {
    case 0:
      return '0';
    case 1:
      return '1';
    case 2:
      return '2';
    case 3:
      return '3';
    case 4:
      return '4';
    case 5:
      return '5';
    case 6:
      return '6';
    case 7:
      return '7';
    case 8:
      return '8';
    case 9:
      return '9';
    default:
      break;
  }
  return ';';
}
AZ_NODISCARD char az_get_max_version(size_t const version) {
  return az_digit_to_char(version / 10);
}
AZ_NODISCARD char az_get_min_version(size_t const version) {
  return az_digit_to_char(version % 10);
}
/**
 * @brief writes a header key and value to a buffer as a 0-terminated string and using a separator
 * span in between. Returns error as soon as any of the write operations fails
 *
 * @param writable_buffer
 * @param p_header
 * @param separator
 * @return az_result
 */
AZ_NODISCARD az_result az_write_to_buffer(
    az_mut_span const writable_buffer,
    az_pair const header,
    az_span const separator) {
  az_span_builder writer = az_span_builder_create(writable_buffer);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, header.key));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, separator));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, header.value));
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
AZ_NODISCARD az_result az_add_header_to_curl_list(
    az_pair const header,
    struct curl_slist ** const p_list,
    az_span const separator) {
  AZ_CONTRACT_ARG_NOT_NULL(p_list);

  // allocate a buffer for header
  size_t const buffer_size = header.key.size + separator.size + header.value.size + 1;
  uint8_t * const p_writable_buffer = (uint8_t *)malloc(buffer_size);
  if (p_writable_buffer == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  char * buffer = (char *)p_writable_buffer;

  // write buffer
  az_mut_span const writable_buffer
      = (az_mut_span){ .begin = p_writable_buffer, .size = buffer_size };
  az_result const write_result = az_write_to_buffer(writable_buffer, header, separator);

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
AZ_NODISCARD az_result
az_build_headers(az_http_request_builder const * const p_hrb, struct curl_slist ** p_headers) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

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
AZ_NODISCARD az_result
az_write_url(az_mut_span const writable_buffer, az_span const url_from_request) {
  az_span_builder writer = az_span_builder_create(writable_buffer);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, url_from_request));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, AZ_STR_ZERO));
  return AZ_OK;
}

// AZ_NODISCARD az_result az_curl_to_http_response(az_span user_buffer) { return AZ_OK; }

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
  az_span_builder * const user_buffer_builder = (az_span_builder *)userp;

  // TODO: format buffer with AZ_RESPONSE_BUILDER
  az_span const span_for_content = (az_span){ .begin = contents, .size = size_with_extra_space };
  AZ_RETURN_IF_FAILED(az_span_builder_append(user_buffer_builder, span_for_content));
  // add 0 so response can be printed
  AZ_RETURN_IF_FAILED(az_span_builder_append(user_buffer_builder, AZ_STR_ZERO));

  // This callback needs to return the response size or curl will consider it as it failed
  return expected_size;
}

/**
 * handles GET request
 */
AZ_NODISCARD az_result az_curl_send_get_request(az_curl * const p_curl) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);

  // send
  AZ_RETURN_IF_CURL_FAILED(curl_easy_perform(p_curl->p_curl));

  return AZ_OK;
}

/**
 * handles POST request. It handles seting up a body for request
 */
AZ_NODISCARD az_result
az_curl_send_post_request(az_curl * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

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
AZ_NODISCARD az_result
setup_headers(az_curl const * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
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
AZ_NODISCARD az_result
setup_url(az_curl const * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

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
  az_mut_span const writable_buffer
      = (az_mut_span){ .begin = p_writable_buffer, .size = url_final_size };
  az_result const result = az_write_url(writable_buffer, az_mut_span_to_span(p_hrb->url));
  CURLcode const set_headers_result = curl_easy_setopt(p_curl->p_curl, CURLOPT_URL, buffer);
  // free used buffer before anything else
  memset(p_writable_buffer, 0, url_final_size);
  free(buffer);

  // handle writing to buffer error
  AZ_RETURN_IF_FAILED(result);
  // handle setting curl url
  AZ_RETURN_IF_CURL_FAILED(set_headers_result);

  return AZ_OK;
}

/**
 * @brief set url the response redirection to user buffer
 *
 * @param p_curl
 * @param p_hrb
 * @return az_result
 */
AZ_NODISCARD az_result setup_response_redirect(
    az_curl const * const p_curl,
    az_span_builder * const response_builder,
    bool const buildRFC7230) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);

  if (buildRFC7230) {
    AZ_RETURN_IF_FAILED(
        az_span_builder_append(response_builder, AZ_CURL_ADAPTER_RESPONSE_PLACEHOLDER));
  }

  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl->p_curl, CURLOPT_WRITEFUNCTION, write_to_span));
  AZ_RETURN_IF_CURL_FAILED(
      curl_easy_setopt(p_curl->p_curl, CURLOPT_WRITEDATA, (void *)response_builder));

  return AZ_OK;
}

/**
 * @brief uses AZ_HTTP_BUILDER to set up CURL request and perform it.
 *
 * @param p_hrb
 * @param response
 * @return az_result
 */
AZ_NODISCARD az_result az_http_client_send_request_impl(
    az_http_request_builder * const p_hrb,
    az_mut_span const * const response,
    bool const buildRFC7230) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  az_curl p_curl;
  az_result result = AZ_ERROR_ARG;
  az_span_builder response_builder = az_span_builder_create(*response);

  AZ_RETURN_IF_FAILED(az_curl_init(&p_curl));

  AZ_RETURN_IF_CURL_FAILED(setup_headers(&p_curl, p_hrb));

  AZ_RETURN_IF_CURL_FAILED(setup_url(&p_curl, p_hrb));

  AZ_RETURN_IF_CURL_FAILED(setup_response_redirect(&p_curl, &response_builder, buildRFC7230));

  if (az_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_GET)) {
    result = az_curl_send_get_request(&p_curl);
  } else if (az_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_POST)) {
    result = az_curl_send_post_request(&p_curl, p_hrb);
  }

  /*
  long response_code;
  AZ_RETURN_IF_CURL_FAILED(
      curl_easy_getinfo(p_curl.p_curl, CURLINFO_RESPONSE_CODE, &response_code));

  size_t http_version;
  curl_easy_getinfo(p_curl.p_curl, CURLINFO_HTTP_VERSION, &http_version);
  */

  AZ_RETURN_IF_FAILED(az_curl_done(&p_curl));
  return result;
}
