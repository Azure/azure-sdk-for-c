// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_curl_adapter.h>
#include <az_http_request.h>

az_result az_http_client_init_impl() {
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1l);
  return AZ_OK;
}

az_result az_http_client_set_URL_impl(az_const_span span) {
  curl_easy_setopt(curl, CURLOPT_URL, span.begin);
  return AZ_OK;
}

az_result az_http_client_download_to_file_impl(az_const_span path) {
  FILE * f;
  f = fopen((const char * restrict)path.begin, "wb");
  int result;

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
  result = curl_easy_perform(curl);
  fclose(f);

  if (result == CURLE_OK) {
    return AZ_OK;
  } else {
    return AZ_ERROR_HTTP_CLIENT_ERROR;
  }
}

az_result az_http_client_clean_impl() {
  curl_easy_cleanup(curl);
  return AZ_OK;
}

az_result az_http_client_send_impl(az_http_request const * const request) {

  // Init client
  az_http_client_init_impl();

  // Set Headers
  az_http_client_set_headers(request);
  return AZ_OK;
}
/*
az_result az_http_client_set_headers(az_http_request const * const request) {
  if (curl) {
    az_pair_iter i = request->headers;
    while (true) {
      az_pair p;
      {
        az_result const result = az_pair_iter_call(&i, &p);
        if (result == AZ_ERROR_EOF) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
      }
      {
        az_const_span joiner_char = AZ_STR(": ");
        int header_len = p.key.size + joiner_char.size + p.valu
      }

      chunk = curl_slist_append(chunk, );

      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p.key));
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, AZ_STR(": ")));
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, p.value));
      AZ_RETURN_IF_FAILED(az_write_span_iter_write(&wi, az_crlf));
    }
  }
  return AZ_OK;
}*/
