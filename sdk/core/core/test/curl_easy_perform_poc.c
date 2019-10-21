// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>

#include <az_http_client.h>
#include <az_json_read.h>

int exit_code = 0;

int main() {
  az_pair const query_array[] = {};
  az_pair_span const query = AZ_SPAN(query_array);
  //
  az_pair const headers_array[] = {};
  az_pair_span const headers = AZ_SPAN(headers_array);
  //
  az_const_span req_body
      = AZ_STR("grant_type=client_credentials&client_id=4317a660-6bfb-4585-9ce9-8f222314879c&"
               "client_secret=O2CT[Y:dkTqblml5V/T]ZEi9x1W1zoBW&resource=https://vault.azure.net");
  az_http_request const request = {
    .method = AZ_STR("POST"),
    .path
    = AZ_STR("https://login.microsoftonline.com/72f988bf-86f1-41af-91ab-2d7cd011db47/oauth2/token"),
    .query = az_pair_span_to_seq(&query),
    .headers = az_pair_span_to_seq(&headers),
    .body = req_body,
  };

  az_span response = { .begin = 0, .size = 0 };

  az_http_client_send_request(&request, &response);

  printf("\nresponse: %s", response.begin);

  az_const_span token = { .begin = 0, .size = 0 };

  az_json_get_object_member_string_value(
      az_span_to_const_span(response), AZ_STR("access_token"), &token);

  uint8_t buffer[2 * 1024];
  az_span const buf = AZ_SPAN(buffer);
  az_write_span_iter builder = az_write_span_iter_create(buf);

  az_write_span_iter_write(&builder, AZ_STR("Bearer "));
  az_write_span_iter_write(&builder, token);
  az_write_span_iter_write(&builder, AZ_STR("\0"));
  az_write_span_iter_result(&builder);

  printf("\n\nToken: %s\n\nResponse: ", buffer);

  az_pair const query_array_get[] = { { .key = AZ_STR("api-version"), .value = AZ_STR("7.0") } };
  az_pair_span const query_get = AZ_SPAN(query_array_get);
  az_pair const headers_array_get[]
      = { { .key = AZ_STR("authorization"), .value = az_span_to_const_span(buf) } };
  az_pair_span const headers_get = AZ_SPAN(headers_array_get);

  az_http_request const request_get = {
    .method = AZ_STR("GET"),
    .path = AZ_STR("https://antk-keyvault.vault.azure.net/secrets/Password"),
    .query = az_pair_span_to_seq(&query_get),
    .headers = az_pair_span_to_seq(&headers_get),
    .body = NULL,
  };

  az_http_client_send_request(&request_get, &response);

  free(response.begin);

  return exit_code;
}
