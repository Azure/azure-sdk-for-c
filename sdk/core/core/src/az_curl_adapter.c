// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

#include <az_curl_slist.h>
#include <az_http_request.h>
#include <az_span.h>
#include <az_span_malloc.h>
#include <az_str.h>

#include <_az_cfg.h>

az_span const AZ_HTTP_REQUEST_BUILDER_HEADER_SEPARATOR = AZ_CONST_STR(": ");

/**
 * @brief writes a header key and value to a buffer as a 0-terminated string and using a separator
 * span in between. Returns error as soon as any of the write operations fails
 *
 * @param writable_buffer pre allocated buffer that will be used to hold header key and value
 * @param header header as an az_pair containing key and value
 * @param separator symbol to be used betwwen key and value
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
 * @param header a key and value representing an http header
 * @param p_list list of headers as curl list
 * @param separator a symbol to be used between key and value for a header
 * @return az_result
 */
AZ_NODISCARD az_result az_add_header_to_curl_list(
    az_pair const header,
    struct curl_slist ** const p_list,
    az_span const separator) {
  AZ_CONTRACT_ARG_NOT_NULL(p_list);

  // allocate a buffer for header
  az_mut_span writable_buffer;
  {
    size_t const buffer_size = header.key.size + separator.size + header.value.size + 1;
    AZ_RETURN_IF_FAILED(az_span_malloc(buffer_size, &writable_buffer));
  }

  // write buffer
  az_result result = az_write_to_buffer(writable_buffer, header, separator);

  // attach header only when write was OK
  if (az_succeeded(result)) {
    char const * const buffer = (char const *)writable_buffer.begin;
    result = az_curl_slist_append(p_list, buffer);
  }

  // at any case, error or OK, free the allocated memory
  az_span_free(&writable_buffer);
  return result;
}

/**
 * @brief loop all the headers from a HTTP request and set each header into easy curl
 *
 * @param p_hrb an http builder request reference
 * @param p_headers list of headers in curl specific list
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
 * @brief writes a url request adds a zero to make it a c-string. Return error if any of the write
 * operations fails.
 *
 * @param writable_buffer a pre-allocated buffer to write http request
 * @param url_from_request a url that is not zero terminated
 * @return az_result
 */
AZ_NODISCARD az_result
az_write_url(az_mut_span const writable_buffer, az_span const url_from_request) {
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
 * @param contents response data from Curl response
 * @param size size of the curl response data
 * @param nmemb number of blocks in response
 * @param userp this represent a structure linked to response by us before
 * @return int
 */
size_t write_to_span(
    void * const contents,
    size_t const size,
    size_t const nmemb,
    void * const userp) {
  size_t const expected_size = size * nmemb;
  az_span_builder * const user_buffer_builder = (az_span_builder *)userp;

  az_span const span_for_content = az_span_from_runtime_array(contents, expected_size);
  AZ_RETURN_IF_FAILED(az_span_builder_append(user_buffer_builder, span_for_content));

  // This callback needs to return the response size or curl will consider it as it failed
  return expected_size;
}

/**
 * handles GET request
 */
AZ_NODISCARD az_result az_curl_send_get_request(CURL * const p_curl) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);

  // send
  AZ_RETURN_IF_CURL_FAILED(curl_easy_perform(p_curl));

  return AZ_OK;
}

/**
 * handles DELETE request
 */
AZ_NODISCARD az_result
az_curl_send_delete_request(CURL * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  AZ_RETURN_IF_FAILED(
      az_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "DELETE")));

  AZ_RETURN_IF_FAILED(az_curl_code_to_result(curl_easy_perform(p_curl)));

  return AZ_OK;
}

/**
 * handles POST request. It handles seting up a body for request
 */
AZ_NODISCARD az_result
az_curl_send_post_request(CURL * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  // Method
  az_mut_span body = { 0 };
  AZ_RETURN_IF_FAILED(az_span_malloc(p_hrb->body.size + AZ_STR_ZERO.size, &body));

  az_mut_span zt_buf = { 0 };
  az_result res_code = az_mut_span_to_str(body, p_hrb->body, &zt_buf);

  if (az_succeeded(res_code)) {
    res_code = az_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_POSTFIELDS, zt_buf.begin));
    if (az_succeeded(res_code)) {
      res_code = az_curl_code_to_result(curl_easy_perform(p_curl));
    }
  }

  az_span_free(&body);
  AZ_RETURN_IF_FAILED(res_code);

  return AZ_OK;
}

size_t _az_curl_upload_read_callback(void * ptr, size_t size, size_t nmemb, void * userdata) {

  az_mut_span * mut_span = (az_mut_span *)userdata;

  size_t curl_size = nmemb * size;

  //Nothing to copy
  if (curl_size < 1 || !(mut_span->size))
    return 0;

  size_t to_copy = (mut_span->size < curl_size) ? mut_span->size : curl_size;
  memcpy(ptr, mut_span->begin, to_copy);
  mut_span->size -= to_copy;
  mut_span->begin += to_copy;

  return to_copy;
}

/**
 * handles POST request. It handles seting up a body for request
 */
AZ_NODISCARD az_result
az_curl_send_upload_request(CURL * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  // Method
  az_mut_span body = { 0 };
  AZ_RETURN_IF_FAILED(az_span_malloc(p_hrb->body.size + AZ_STR_ZERO.size, &body));

  az_mut_span zt_buf = { 0 };
  az_result res_code = az_mut_span_to_str(body, p_hrb->body, &zt_buf);

  if (az_succeeded(res_code)) {
    res_code = az_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 1L));
    if (az_succeeded(res_code)) {
      res_code = az_curl_code_to_result(
          curl_easy_setopt(p_curl, CURLOPT_READFUNCTION, _az_curl_upload_read_callback));
      if (az_succeeded(res_code)) {

        //Setup the request to pass the body in as the data stream to read
        res_code = az_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_READDATA, body));
        
        if (az_succeeded(res_code)) {

          //Set the size of the upload
          res_code = az_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_INFILESIZE, (curl_off_t)body.size));
          
          // Do the curl work
          if (az_succeeded(res_code)) {
            res_code = az_curl_code_to_result(curl_easy_perform(p_curl));
          }
        }
      }
    }
  }

  az_span_free(&body);
  AZ_RETURN_IF_FAILED(res_code);

  return AZ_OK;
}

/**
 * @brief finds out if there are headers in the request and add them to curl header list
 *
 * @param p_curl curl specific structure to send a request
 * @param p_hrb an http request builder
 * @return az_result
 */
AZ_NODISCARD az_result
setup_headers(CURL * const p_curl, az_http_request_builder const * const p_hrb) {
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
  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_HTTPHEADER, p_list));

  return AZ_OK;
}

/**
 * @brief set url for the request
 *
 * @param p_curl specific curl struct to send a request
 * @param p_hrb an az http request builder holding all data to send request
 * @return az_result
 */
AZ_NODISCARD az_result setup_url(CURL * const p_curl, az_http_request_builder const * const p_hrb) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);

  az_mut_span writable_buffer;
  {
    // set URL as 0-terminated str
    size_t const extra_space_for_zero = AZ_STR_ZERO.size;
    size_t const url_final_size = p_hrb->url_builder.length + extra_space_for_zero;
    // allocate buffer to add \0
    AZ_RETURN_IF_FAILED(az_span_malloc(url_final_size, &writable_buffer));
  }

  // write url in buffer (will add \0 at the end)
  az_result result = az_write_url(writable_buffer, az_span_builder_result(&p_hrb->url_builder));

  if (az_succeeded(result)) {
    char * buffer = (char *)writable_buffer.begin;
    result = az_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_URL, buffer));
  }

  // free used buffer before anything else
  az_mut_span_fill(writable_buffer, 0);
  az_span_free(&writable_buffer);

  return result;
}

/**
 * @brief set url the response redirection to user buffer
 *
 * @param p_curl specif curl structure used to send http request
 * @param response_builder an http request builder holding all http request data
 * @param buildRFC7230 when it is set to true, response will be parsed following RFC 7230
 * @return az_result
 */
AZ_NODISCARD az_result setup_response_redirect(
    CURL * const p_curl,
    az_span_builder * const response_builder,
    bool const buildRFC7230) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);

  if (buildRFC7230) {
    AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_HEADERFUNCTION, write_to_span));
    AZ_RETURN_IF_CURL_FAILED(
        curl_easy_setopt(p_curl, CURLOPT_HEADERDATA, (void *)response_builder));
  }

  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_WRITEFUNCTION, write_to_span));
  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_WRITEDATA, (void *)response_builder));

  return AZ_OK;
}

/**
 * @brief use this method to group all the actions that we do with CURL so we can clean it after it
 * no matter is there is an error at any step.
 *
 * @param p_curl curl specific structure used to send an http request
 * @param p_hrb http builder with specific data to build an http request
 * @param response pre-allocated buffer where to write http response
 * @param buildRFC7230 when true, response will follow RFC 7230
 * @return AZ_OK if request was sent and a response was received
 */
AZ_NODISCARD az_result az_http_client_send_request_impl_process(
    CURL * p_curl,
    az_http_request_builder * const p_hrb,
    az_http_response * const response,
    bool const buildRFC7230) {
  az_result result = AZ_ERROR_ARG;

  AZ_RETURN_IF_CURL_FAILED(setup_headers(p_curl, p_hrb));

  AZ_RETURN_IF_CURL_FAILED(setup_url(p_curl, p_hrb));

  AZ_RETURN_IF_CURL_FAILED(setup_response_redirect(p_curl, &response->builder, buildRFC7230));

  if (az_span_is_equal(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_GET)) {
    result = az_curl_send_get_request(p_curl);
  } else if (az_span_is_equal(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_POST)) {
    result = az_curl_send_post_request(p_curl, p_hrb);
  } else if (az_span_is_equal(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_DELETE)) {
    result = az_curl_send_delete_request(p_curl, p_hrb);
  } else if (az_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_PUT)) {
    result = az_curl_send_upload_request(p_curl, p_hrb);
  } else {
    return AZ_ERROR_HTTP_INVALID_METHOD_VERB;
  }

  // make sure to set the end of the body response as the end of the complete response
  if (az_succeeded(result)) {
    AZ_RETURN_IF_FAILED(az_span_builder_append(&response->builder, AZ_STR_ZERO));
  }
  return AZ_OK;
}

/**
 * @brief uses AZ_HTTP_BUILDER to set up CURL request and perform it.
 *
 * @param p_hrb an internal http builder with data to build and send http request
 * @param response pre-allocated buffer where http response will be written
 * @return az_result
 */
AZ_NODISCARD az_result az_http_client_send_request_impl(
    az_http_request_builder * const p_hrb,
    az_http_response * const response,
    bool const buildRFC7230) {
  AZ_CONTRACT_ARG_NOT_NULL(p_hrb);
  AZ_CONTRACT_ARG_NOT_NULL(response);

  CURL * p_curl = NULL;

  // init curl
  AZ_RETURN_IF_FAILED(az_curl_init(&p_curl));

  // process request
  az_result process_result
      = az_http_client_send_request_impl_process(p_curl, p_hrb, response, buildRFC7230);

  // no matter if error or not, call curl done before returning to let curl clean everything
  AZ_RETURN_IF_FAILED(az_curl_done(&p_curl));

  return process_result;
}
