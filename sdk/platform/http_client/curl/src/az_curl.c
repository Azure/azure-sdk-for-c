// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>

#include <stdlib.h>

#include <curl/curl.h>

#include <_az_cfg.h>

/*Copying AZ_CONTRACT on purpose from AZ_CORE because 3rd parties can define this and should not
 * depend on internal CORE headers */
#define AZ_CONTRACT(condition, error) \
  do { \
    if (!(condition)) { \
      return error; \
    } \
  } while (0)

#define AZ_CONTRACT_ARG_NOT_NULL(arg) AZ_CONTRACT((arg) != NULL, AZ_ERROR_ARG)

static AZ_NODISCARD az_result _az_span_malloc(int32_t size, az_span * out) {
  AZ_CONTRACT_ARG_NOT_NULL(out);

  uint8_t * const p = (uint8_t *)malloc((size_t)size);
  if (p == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  *out = az_span_init(p, 0, size);
  return AZ_OK;
}

static void _az_span_free(az_span * p) {
  if (p == NULL) {
    return;
  }
  free(az_span_ptr(*p));
  *p = az_span_null();
}

/**
 * Converts CURLcode to az_result.
 */
AZ_NODISCARD az_result _az_http_client_curl_code_to_result(CURLcode code) {
  switch (code) {
    case CURLE_OK:
      return AZ_OK;

    case CURLE_WRITE_ERROR:
      return AZ_ERROR_HTTP_RESPONSE_OVERFLOW;

    case CURLE_COULDNT_RESOLVE_HOST:
      return AZ_ERROR_HTTP_RESPONSE_COULDNT_RESOLVE_HOST;

    default:
      // let any other error code be an HTTP PAL ERROR
      return AZ_ERROR_HTTP_PAL;
  }
}

// returning AZ error on CURL Error
#define AZ_RETURN_IF_CURL_FAILED(exp) AZ_RETURN_IF_FAILED(_az_http_client_curl_code_to_result(exp))

AZ_NODISCARD AZ_INLINE az_result _az_http_client_curl_init(CURL ** out) {
  *out = curl_easy_init();
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result _az_http_client_curl_done(CURL ** pp) {
  AZ_CONTRACT_ARG_NOT_NULL(pp);
  AZ_CONTRACT_ARG_NOT_NULL(*pp);

  curl_easy_cleanup(*pp);
  *pp = NULL;
  return AZ_OK;
}

/**
 * @brief writes a header key and value to a buffer as a 0-terminated string and using a separator
 * span in between. Returns error as soon as any of the write operations fails
 *
 * @param writable_buffer pre allocated buffer that will be used to hold header key and value
 * @param header header as an az_pair containing key and value
 * @param separator symbol to be used between key and value
 * @return az_result
 */
static AZ_NODISCARD az_result
_az_span_append_header_to_buffer(az_span writable_buffer, az_pair header, az_span separator) {
  AZ_RETURN_IF_FAILED(az_span_append(writable_buffer, header.key, &writable_buffer));
  AZ_RETURN_IF_FAILED(az_span_append(writable_buffer, separator, &writable_buffer));
  AZ_RETURN_IF_FAILED(az_span_append(writable_buffer, header.value, &writable_buffer));
  AZ_RETURN_IF_FAILED(az_span_append(writable_buffer, AZ_SPAN_FROM_STR("\0"), &writable_buffer));

  return AZ_OK;
}

static AZ_NODISCARD az_result
_az_http_client_curl_slist_append(struct curl_slist ** self, char const * str) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(str);

  struct curl_slist * const p_list = curl_slist_append(*self, str);
  if (p_list == NULL) {
    return AZ_ERROR_HTTP_PAL;
  }
  *self = p_list;
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
static AZ_NODISCARD az_result _az_http_client_curl_add_header_to_curl_list(
    az_pair header,
    struct curl_slist ** p_list,
    az_span separator) {
  AZ_CONTRACT_ARG_NOT_NULL(p_list);

  // allocate a buffer for header
  az_span writable_buffer;
  {
    int32_t const buffer_size = az_span_length(header.key) + az_span_length(separator)
        + az_span_length(header.value) + az_span_length(AZ_SPAN_FROM_STR("\0"));

    AZ_RETURN_IF_FAILED(_az_span_malloc(buffer_size, &writable_buffer));
  }

  // write buffer
  az_result result = _az_span_append_header_to_buffer(writable_buffer, header, separator);

  // attach header only when write was OK
  if (az_succeeded(result)) {
    char const * const buffer = (char const *)az_span_ptr(writable_buffer);
    result = _az_http_client_curl_slist_append(p_list, buffer);
  }

  // at any case, error or OK, free the allocated memory
  _az_span_free(&writable_buffer);
  return result;
}

/**
 * @brief loop all the headers from a HTTP request and set each header into easy curl
 *
 * @param p_request an http builder request reference
 * @param p_headers list of headers in curl specific list
 * @return az_result
 */
static AZ_NODISCARD az_result
_az_http_client_curl_build_headers(_az_http_request * p_request, struct curl_slist ** p_headers) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  az_pair header;
  for (int32_t offset = 0;
       offset < (int32_t)(az_span_length(p_request->_internal.headers) / sizeof(az_pair));
       ++offset) {
    AZ_RETURN_IF_FAILED(az_http_request_get_header(p_request, offset, &header));
    AZ_RETURN_IF_FAILED(
        _az_http_client_curl_add_header_to_curl_list(header, p_headers, AZ_SPAN_FROM_STR(": ")));
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
static AZ_NODISCARD az_result
_az_http_client_curl_append_url(az_span writable_buffer, az_span url_from_request) {

  AZ_RETURN_IF_FAILED(az_span_append(writable_buffer, url_from_request, &writable_buffer));
  AZ_RETURN_IF_FAILED(az_span_append(writable_buffer, AZ_SPAN_FROM_STR("\0"), &writable_buffer));

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
static size_t _az_http_client_curl_write_to_span(
    void * contents,
    size_t size,
    size_t nmemb,
    void * userp) {
  size_t const expected_size = size * nmemb;
  az_span * const user_buffer_builder = (az_span *)userp;

  az_span const span_for_content
      = az_span_init((uint8_t *)contents, (int32_t)expected_size, (int32_t)expected_size);
  AZ_RETURN_IF_FAILED(az_span_append(*user_buffer_builder, span_for_content, user_buffer_builder));

  // This callback needs to return the response size or curl will consider it as it failed
  return expected_size;
}

/**
 * handles GET request
 */
static AZ_NODISCARD az_result _az_http_client_curl_send_get_request(CURL * p_curl) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);

  // send
  AZ_RETURN_IF_CURL_FAILED(curl_easy_perform(p_curl));

  return AZ_OK;
}

/**
 * handles DELETE request
 */
static AZ_NODISCARD az_result
_az_http_client_curl_send_delete_request(CURL * p_curl, _az_http_request const * p_request) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  AZ_RETURN_IF_FAILED(_az_http_client_curl_code_to_result(
      curl_easy_setopt(p_curl, CURLOPT_CUSTOMREQUEST, "DELETE")));

  AZ_RETURN_IF_FAILED(_az_http_client_curl_code_to_result(curl_easy_perform(p_curl)));

  return AZ_OK;
}

/**
 * handles POST request. It handles seting up a body for request
 */
static AZ_NODISCARD az_result
_az_http_client_curl_send_post_request(CURL * p_curl, _az_http_request const * p_request) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  // Method
  az_span body = { 0 };
  int32_t const required_length
      = az_span_length(p_request->_internal.body) + az_span_length(AZ_SPAN_FROM_STR("\0"));

  AZ_RETURN_IF_FAILED(_az_span_malloc(required_length, &body));

  char * b = (char *)az_span_ptr(body);
  az_result res_code = az_span_to_str(b, required_length, p_request->_internal.body);

  if (az_succeeded(res_code)) {
    res_code = _az_http_client_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_POSTFIELDS, b));
    if (az_succeeded(res_code)) {
      res_code = _az_http_client_curl_code_to_result(curl_easy_perform(p_curl));
    }
  }

  _az_span_free(&body);
  AZ_RETURN_IF_FAILED(res_code);

  return AZ_OK;
}

/**
 * @brief UPLOAD requests are done via callbacks.  The callback is passed in a buffer address which
 * is filled with the userdata content. The callback will occur until the callback returns 0 (no
 * more data). The callback will return CURL_READFUNC_ABORT should an error occur.  This in turn
 * terminates the POST request.
 *
 * @param dst Destination address buffer
 * @param size Size of an item
 * @param nmemb Number of items to copy
 * @param userdata Source data to upload
 *                 Passed as the pointer to an az_span
 * @return int
 */
static int32_t _az_http_client_curl_upload_read_callback(
    void * dst,
    size_t size,
    size_t nmemb,
    void * userdata) {

  az_span * upload_content = (az_span *)userdata;

  // Calculate the size of the *dst buffer
  int32_t dst_buffer_size = (int32_t)(nmemb * size);

  // Terminate the upload if the destination buffer is too small
  if (dst_buffer_size < 1)
    return CURL_READFUNC_ABORT;

  int32_t userdata_length = az_span_length(*upload_content);

  // Return if nothing to copy
  if (userdata_length < 1)
    return CURLE_OK; // Success, all bytes copied

  int32_t size_of_copy = (userdata_length < dst_buffer_size) ? userdata_length : dst_buffer_size;

  memcpy(dst, az_span_ptr(*upload_content), size_of_copy);

  // Update the userdata span
  //  ptr will point to remaining data to be copied
  //  length and capacity are set to the size of the remaining content
  az_result result = az_span_slice(*upload_content, dst_buffer_size, -1, upload_content);

  // Upon failure terminate the upload
  // per CURL documentation, "The read callback may return CURL_READFUNC_ABORT to stop the current
  // operation immediately,
  //          resulting in a CURLE_ABORTED_BY_CALLBACK error code from the transfer."
  //
  if (az_failed(result)) {
    return CURL_READFUNC_ABORT;
  }

  return size_of_copy;
}

/**
 * Perform an UPLOAD or PUT request.
 * As of CURL 7.12.1 CURLOPT_PUT is deprecated.  PUT requests should be made using CURLOPT_UPLOAD
 */
static AZ_NODISCARD az_result
_az_http_client_curl_send_upload_request(CURL * p_curl, _az_http_request const * p_request) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  az_span body = p_request->_internal.body;

  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_UPLOAD, 1L));
  AZ_RETURN_IF_CURL_FAILED(
      curl_easy_setopt(p_curl, CURLOPT_READFUNCTION, _az_http_client_curl_upload_read_callback));

  // Setup the request to pass body into the read callback
  // The read callback receives the address of body
  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_READDATA, &body));

  // Set the size of the upload
  AZ_RETURN_IF_CURL_FAILED(
      curl_easy_setopt(p_curl, CURLOPT_INFILESIZE, (curl_off_t)az_span_length(body)));

  // Do the curl work
  // curl_easy_perform does not return until the CURLOPT_READFUNCTION callbacks complete.
  AZ_RETURN_IF_CURL_FAILED(curl_easy_perform(p_curl));

  return AZ_OK;
}

/**
 * @brief finds out if there are headers in the request and add them to curl header list
 *
 * @param p_curl curl specific structure to send a request
 * @param p_request an http request builder
 * @return az_result
 */
static AZ_NODISCARD az_result
_az_http_client_curl_setup_headers(CURL * p_curl, _az_http_request * p_request) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  if (az_span_length(p_request->_internal.headers) == 0) {
    // no headers, no need to set it up
    return AZ_OK;
  }

  // creates a slist for bulding curl headers
  struct curl_slist * p_list = NULL;
  // build headers into a slist as curl is expecting
  AZ_RETURN_IF_FAILED(_az_http_client_curl_build_headers(p_request, &p_list));
  // set all headers from slist
  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_HTTPHEADER, p_list));

  return AZ_OK;
}

/**
 * @brief set url for the request
 *
 * @param p_curl specific curl struct to send a request
 * @param p_request an az http request builder holding all data to send request
 * @return az_result
 */
static AZ_NODISCARD az_result
_az_http_client_curl_setup_url(CURL * p_curl, _az_http_request const * p_request) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  az_span writable_buffer;
  {
    // Add 1 for 0-terminated str
    int32_t const url_final_size = az_span_length(p_request->_internal.url) + 1;

    // allocate buffer to add \0
    AZ_RETURN_IF_FAILED(_az_span_malloc(url_final_size, &writable_buffer));
  }

  // write url in buffer (will add \0 at the end)
  az_result result = _az_http_client_curl_append_url(writable_buffer, p_request->_internal.url);

  if (az_succeeded(result)) {
    char * buffer = (char *)az_span_ptr(writable_buffer);
    result = _az_http_client_curl_code_to_result(curl_easy_setopt(p_curl, CURLOPT_URL, buffer));
  }

  // free used buffer before anything else
  memset(az_span_ptr(writable_buffer), 0, az_span_capacity(writable_buffer));
  _az_span_free(&writable_buffer);

  return result;
}

/**
 * @brief set url the response redirection to user buffer
 *
 * @param p_curl specif curl structure used to send http request
 * @param response_builder an http request builder holding all http request data
 * @return az_result
 */
static AZ_NODISCARD az_result
_az_http_client_curl_setup_response_redirect(CURL * p_curl, az_span * response_builder) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);

  AZ_RETURN_IF_CURL_FAILED(
      curl_easy_setopt(p_curl, CURLOPT_HEADERFUNCTION, _az_http_client_curl_write_to_span));

  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_HEADERDATA, (void *)response_builder));

  AZ_RETURN_IF_CURL_FAILED(
      curl_easy_setopt(p_curl, CURLOPT_WRITEFUNCTION, _az_http_client_curl_write_to_span));

  AZ_RETURN_IF_CURL_FAILED(curl_easy_setopt(p_curl, CURLOPT_WRITEDATA, (void *)response_builder));

  return AZ_OK;
}

/**
 * @brief use this method to group all the actions that we do with CURL so we can clean it after it
 * no matter is there is an error at any step.
 *
 * @param p_curl curl specific structure used to send an http request
 * @param p_request http builder with specific data to build an http request
 * @param response pre-allocated buffer where to write http response

 * @return AZ_OK if request was sent and a response was received
 */
static AZ_NODISCARD az_result _az_http_client_curl_send_request_impl_process(
    CURL * p_curl,
    _az_http_request * p_request,
    az_http_response * response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_curl);
  AZ_CONTRACT_ARG_NOT_NULL(p_request);

  az_result result = AZ_ERROR_ARG;

  AZ_RETURN_IF_FAILED(_az_http_client_curl_setup_headers(p_curl, p_request));

  AZ_RETURN_IF_FAILED(_az_http_client_curl_setup_url(p_curl, p_request));

  AZ_RETURN_IF_FAILED(
      _az_http_client_curl_setup_response_redirect(p_curl, &response->_internal.http_response));

  if (az_span_is_equal(p_request->_internal.method, az_http_method_get())) {
    result = _az_http_client_curl_send_get_request(p_curl);
  } else if (az_span_is_equal(p_request->_internal.method, az_http_method_post())) {
    result = _az_http_client_curl_send_post_request(p_curl, p_request);
  } else if (az_span_is_equal(p_request->_internal.method, az_http_method_delete())) {
    result = _az_http_client_curl_send_delete_request(p_curl, p_request);
  } else if (az_span_is_equal(p_request->_internal.method, az_http_method_put())) {
    // As of CURL 7.12.1 CURLOPT_PUT is deprecated.  PUT requests should be made using
    // CURLOPT_UPLOAD
    result = _az_http_client_curl_send_upload_request(p_curl, p_request);
  } else {
    return AZ_ERROR_HTTP_INVALID_METHOD_VERB;
  }

  // make sure to set the end of the body response as the end of the complete response
  if (az_succeeded(result)) {
    AZ_RETURN_IF_FAILED(az_span_append(
        response->_internal.http_response,
        AZ_SPAN_FROM_STR("\0"),
        &response->_internal.http_response));
  }
  return result;
}

/**
 * @brief uses AZ_HTTP_BUILDER to set up CURL request and perform it.
 *
 * @param p_request an internal http builder with data to build and send http request
 * @param p_response pre-allocated buffer where http response will be written
 * @return az_result
 */
AZ_NODISCARD az_result
az_http_client_send_request(_az_http_request * p_request, az_http_response * p_response) {
  AZ_CONTRACT_ARG_NOT_NULL(p_request);
  AZ_CONTRACT_ARG_NOT_NULL(p_response);

  CURL * p_curl = NULL;

  // init curl
  AZ_RETURN_IF_FAILED(_az_http_client_curl_init(&p_curl));

  // process request
  az_result process_result
      = _az_http_client_curl_send_request_impl_process(p_curl, p_request, p_response);

  // no matter if error or not, call curl done before returning to let curl clean everything
  AZ_RETURN_IF_FAILED(_az_http_client_curl_done(&p_curl));

  return process_result;
}
