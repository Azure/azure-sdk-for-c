// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

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
    az_const_span separator) {
  az_write_span_iter writer = az_write_span_iter_create(writable_buffer);
  az_result write_result;
  write_result = az_write_span_iter_write(&writer, p_header->key);
  if (write_result != AZ_OK) {
    return AZ_ERROR_ARG;
  }
  write_result = az_write_span_iter_write(&writer, separator);
  if (write_result != AZ_OK) {
    return AZ_ERROR_ARG;
  }
  write_result = az_write_span_iter_write(&writer, p_header->value);
  if (write_result != AZ_OK) {
    return AZ_ERROR_ARG;
  }
  write_result = az_write_span_iter_write(&writer, AZ_STR("\0"));
  if (write_result != AZ_OK) {
    return AZ_ERROR_ARG;
  }
  az_write_span_iter_result(&writer);

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
  int16_t buffer_size = p_header->key.size + separator.size + p_header->value.size + 1;
  uint8_t * const p_writable_buffer = (uint8_t * const)malloc(buffer_size);
  if (p_writable_buffer == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  char * buffer = (char *)p_writable_buffer;

  // write buffer
  az_span const writable_buffer = AZ_SPAN(p_writable_buffer);
  az_result write_result = az_write_to_buffer(writable_buffer, p_header, separator);

  // attach header only when write was OK
  if (write_result == AZ_OK) {
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
    az_http_request_builder const * const p_hrb,
    az_curl_headers_list * p_headers) {
  az_const_span separator = AZ_STR(": ");
  // get pointer to first header
  az_pair * p_header = p_hrb->headers_info.headers_start;

  for (int8_t offset = 1; offset < p_hrb->headers_info.size; offset++) {
    AZ_RETURN_IF_FAILED(az_add_header_to_curl_list(p_header, p_headers, separator));
    p_header += 1;
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
  az_write_span_iter writer = az_write_span_iter_create(writable_buffer);
  az_result write_result;
  write_result = az_write_span_iter_write(&writer, url_from_request);
  if (write_result != AZ_OK) {
    return AZ_ERROR_ARG;
  }
  write_result = az_write_span_iter_write(&writer, AZ_STR("\0"));
  if (write_result != AZ_OK) {
    return AZ_ERROR_ARG;
  }
  return AZ_OK;
}

/**
 * @brief Gets the url from request and writes it to a new buffer as a c-string. Then sets curl url
 * whit this string. Memory for the new buffer is free after setting curl value or if write
 * operation fails.
 *
 * @param p_curl
 * @param p_hrb
 * @return az_result
 */
az_result az_build_url(az_curl * const p_curl, az_http_request_builder const * const p_hrb) {
  // allocate a new buffer for url with 0 terminated
  uint8_t * const p_writable_buffer = (uint8_t * const)malloc(p_hrb->url_info.size + 1);
  if (p_writable_buffer == NULL) {
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  char * buffer = (char *)p_writable_buffer;

  // get a new span for url inside request buffer
  az_const_span const url_from_request = { .begin = &p_hrb->buffer, .size = p_hrb->url_info.size };
  az_span const writable_buffer = AZ_SPAN(p_writable_buffer);

  az_result write_result = az_write_url(writable_buffer, url_from_request);

  if (write_result == AZ_OK) {
    curl_easy_setopt(p_curl->p_curl, CURLOPT_URL, buffer);
  }
  free(p_writable_buffer);
  return write_result;
}

/**
 * handles GET request
 */
az_result az_curl_send_request(
    az_curl * const p_curl,
    az_http_request_builder const * const p_hrb) {
  (void)p_hrb;
  // creates a slist for bulding curl headers
  az_curl_headers_list headers = {
    .p_list = NULL,
  };
  // build headers into a slist as curl is expecting
  AZ_RETURN_IF_FAILED(az_build_headers(p_hrb, &headers));
  // set all headers from slist
  curl_easy_setopt(p_curl->p_curl, CURLOPT_HTTPHEADER, headers.p_list);

  // set URL
  AZ_RETURN_IF_FAILED(az_build_url(p_curl->p_curl, p_hrb));

  CURLcode res = curl_easy_perform(p_curl->p_curl);
  if (res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

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
    az_http_request_builder const * const p_hrb,
    az_span * const response) {
  az_curl p_curl;
  AZ_RETURN_IF_FAILED(az_curl_init(&p_curl));
  az_result result;

  if (az_const_span_eq(p_hrb->method_verb, AZ_STR("GET"))) {
    result = az_curl_send_request(&p_curl, p_hrb);
  } else if (az_const_span_eq(p_hrb->method_verb, AZ_STR("GET"))) {
    result = az_curl_post_request(&p_curl, p_hrb, response);
  }

  az_curl_done(&p_curl);
  return result;
}