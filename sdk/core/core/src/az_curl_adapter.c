// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>

typedef struct {
  struct curl_slist * p_list;
} az_headers_data;

/**
 * handles GET request
 */
az_result az_curl_send_request(
    az_curl * const p_curl,
    az_http_request_builder const * const p_hrb) {
  (void)p_hrb;
  // creates a slist for bulding curl headers
  az_headers_data headers = {
    .p_list = NULL,
  };
  // build headers into a slist as curl is expecting
  // TODO: AZ_RETURN_IF_FAILED(az_build_headers(p_hrb, &headers));
  // set all headers from slist
  curl_easy_setopt(p_curl->p_curl, CURLOPT_HTTPHEADER, headers.p_list);

  // set URL
  // char * url;
  // TODO: AZ_RETURN_IF_FAILED(az_http_url_to_new_str(p_hrb, &url));
  // curl_easy_setopt(p_curl->p_curl, CURLOPT_URL, url);
  // free(url);

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