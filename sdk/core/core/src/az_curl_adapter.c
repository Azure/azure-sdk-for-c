// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

#include <az_callback.h>
#include <az_http_request.h>

#include <_az_cfg.h>

typedef struct {
  struct curl_slist * p_list;
} az_curl_headers_list;

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
    az_pair * const p_header,
    az_const_span const separator) {
  az_span_builder writer = az_span_builder_create(writable_buffer);
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, p_header->key));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, separator));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, p_header->value));
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, AZ_STR("\0")));
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
    az_pair * const p_header,
    az_curl_headers_list * const p_headers,
    az_const_span separator) {
  // allocate a buffet for header
  int16_t const buffer_size = p_header->key.size + separator.size + p_header->value.size + 1;
  uint8_t * const p_writable_buffer = (uint8_t * const)malloc(buffer_size);
  if (p_writable_buffer == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  char * buffer = (char *)p_writable_buffer;

  // write buffer
  az_span const writable_buffer
      = (az_span const){ .begin = p_writable_buffer, .size = buffer_size };
  az_result const write_result = az_write_to_buffer(writable_buffer, p_header, separator);

  // attach header only when write was OK
  if (az_succeeded(write_result)) {
    p_headers->p_list = curl_slist_append(p_headers->p_list, buffer);
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
    az_http_request_builder * const p_hrb,
    az_curl_headers_list * p_headers) {
  az_const_span separator = AZ_STR(": ");
  // get pointer to first header

  az_pair header;
  for (uint16_t offset = 0; offset < p_hrb->headers_end; ++offset) {
    AZ_RETURN_IF_FAILED(az_http_request_builder_get_header(p_hrb, offset, &header));
    AZ_RETURN_IF_FAILED(az_add_header_to_curl_list(&header, p_headers, separator));
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
  AZ_RETURN_IF_FAILED(az_span_builder_append(&writer, AZ_STR("\0")));
  return AZ_OK;
}

/**
 * handles GET request
 */
az_result az_curl_send_request(az_curl * const p_curl, az_http_request_builder * const p_hrb) {
  // creates a slist for bulding curl headers
  az_curl_headers_list headers = {
    .p_list = NULL,
  };
  // build headers into a slist as curl is expecting
  AZ_RETURN_IF_FAILED(az_build_headers(p_hrb, &headers));
  // set all headers from slist
  curl_easy_setopt(p_curl->p_curl, CURLOPT_HTTPHEADER, headers.p_list);

  // set URL as 0-terminated str
  size_t const extra_space_for_zero = (size_t)sizeof("\0");
  size_t const url_final_size = p_hrb->url.size + extra_space_for_zero;
  uint8_t * const p_writable_buffer = (uint8_t * const)malloc(url_final_size);
  char * buffer = (char *)p_writable_buffer;
  az_span const writable_buffer
      = (az_span const){ .begin = p_writable_buffer, .size = url_final_size };
  az_result result = az_write_url(writable_buffer, az_span_to_const_span(p_hrb->url));
  if (az_succeeded(result)) {
    curl_easy_setopt(p_curl->p_curl, CURLOPT_URL, buffer);
  }
  memset(p_writable_buffer, 0, url_final_size);
  free(buffer);

  // send
  CURLcode res = curl_easy_perform(p_curl->p_curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
  }

  // TODO: would this be part of done function instead?
  curl_slist_free_all(headers.p_list);

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
int write_to_span(void * contents, size_t size, size_t nmemb, void * userp) {
  size_t realsize = size * nmemb + 1;
  az_span * const mem = (az_span * const)userp;

  if (mem->size == 0) {
    mem->begin = malloc(realsize);
    mem->size = realsize;
  } else if (mem->size < realsize) {
    uint8_t * ptr = realloc(mem->begin, realsize);
    if (ptr == NULL) {
      /* out of memory! */
      printf("not enough memory (realloc returned NULL)\n");
      return AZ_ERROR_ARG;
    }
    mem->begin = ptr;
  }

  memcpy(&(mem->begin[0]), contents, realsize);
  mem->size += realsize;
  mem->begin[mem->size] = 0;

  // This callback needs to return the response size or curl will consider it as it failed
  return (int)realsize - 1;
}

/**
 * handles POST request
 */
az_result az_curl_post_request(
    az_curl * const p_curl,
    az_http_request_builder const * const p_hrb,
    az_span * const response) {
  (void)p_hrb;
  // Method
  // TODO: curl_easy_setopt(p_curl->p_curl, CURLOPT_POSTFIELDS, p_hrb->body.begin);

  // URL
  // char * url;
  // TODO: AZ_RETURN_IF_FAILED(az_http_url_to_new_str(p_hrb, &url));
  // curl_easy_setopt(p_curl->p_curl, CURLOPT_URL, url);
  // free(url);

  curl_easy_setopt(p_curl->p_curl, CURLOPT_WRITEFUNCTION, write_to_span);
  curl_easy_setopt(p_curl->p_curl, CURLOPT_WRITEDATA, (void *)response);

  CURLcode res = curl_easy_perform(p_curl->p_curl);

  if (res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

  return AZ_OK;
}

az_result az_http_client_send_request_impl(
    az_http_request_builder * const p_hrb,
    az_span * const response) {
  az_curl p_curl;
  AZ_RETURN_IF_FAILED(az_curl_init(&p_curl));
  az_result result;

  if (az_const_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_GET)) {
    result = az_curl_send_request(&p_curl, p_hrb);
  } else if (az_const_span_eq(p_hrb->method_verb, AZ_HTTP_METHOD_VERB_POST)) {
    result = az_curl_post_request(&p_curl, p_hrb, response);
  }

  az_curl_done(&p_curl);
  return result;
}