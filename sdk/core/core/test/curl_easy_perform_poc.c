// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>

#include <az_http_client.h>
#include <az_json_read.h>

int exit_code = 0;

int main() {

  /****** -------------  Create request from arrays ---------******/
  az_pair const query_array[] = { { .key = AZ_STR("key"), .value = AZ_STR("value") } };
  az_pair_span const query = AZ_SPAN(query_array);
  az_pair const header_array[] = { { .key = AZ_STR("key"), .value = AZ_STR("value") } };
  az_pair_span const header = AZ_SPAN(header_array);

  az_const_span req_body
      = AZ_STR("grant_type=client_credentials&client_id=4317a660-6bfb-4585-9ce9-8f222314879c&"
               "client_secret=O2CT[Y:dkTqblml5V/T]ZEi9x1W1zoBW&resource=https://vault.azure.net");
  az_http_request const request = {
    .method = AZ_STR("POST"),
    .path
    = AZ_STR("https://login.microsoftonline.com/72f988bf-86f1-41af-91ab-2d7cd011db47/oauth2/token"),
    .query = az_pair_span_to_seq_callback(&query),
    .headers = az_pair_span_to_seq_callback(&header),
    .body = req_body,
  };

  /****** -------------  Create span for response ---------******/
  az_span response = { .begin = 0, .size = 0 };

  /****** -------------  send request ---------******/
  // CURL adapter will allocate memory and will have response pointing to it
  az_http_client_send_request(&request, &response);

  /****** -------------  print response from server ---------******/
  printf("\nresponse: %s", response.begin);

  /****** -------------  Parse response to get token for auth ---------******/
  az_const_span token = { .begin = 0, .size = 0 };
  az_json_get_object_member_string_value(
      az_span_to_const_span(response), AZ_STR("access_token"), &token);

  /****** -------------  Create buffer for header auth ---------******/
  // can't print token right now since it is not 0-terminated
  uint8_t const buffer[2000];

  /****** -------------  use Span builder to concatenate ---------******/
  az_span const buf = AZ_SPAN(buffer);
  az_write_span_iter builder = az_write_span_iter_create(buf);
  az_write_span_iter_write(&builder, AZ_STR("Bearer "));
  az_write_span_iter_write(&builder, token);
  az_write_span_iter_write(&builder, AZ_STR("\0")); // add a 0 so it can be printed and used by Curl
  az_write_span_iter_result(&builder);

  /****** -------------  now we can print concatenated token ---------******/
  printf("\n\nToken: %s", buffer);

  /****** -------------  Build GET request from arrays and with auth token  ---------******/
  // query args
  az_pair const query_array_get[] = { { .key = AZ_STR("api-version"), .value = AZ_STR("7.0") } };
  az_pair_span const query_get = AZ_SPAN(query_array_get);
  // headers
  az_pair const headers_array_get[]
      = { { .key = AZ_STR("authorization"), .value = az_span_to_const_span(buf) } };
  az_pair_span const headers_get = AZ_SPAN(headers_array_get);

  az_http_request const request_get = {
    .method = AZ_STR("GET"),
    .path = AZ_STR("https://antk-keyvault.vault.azure.net/secrets/Password"),
    .query = az_pair_span_to_seq_callback(&query_get),
    .headers = az_pair_span_to_seq_callback(&headers_get),
    .body = NULL, // wont use body
  };

  /****** -------------  send request with header auth ---------******/
  printf("\n\nResponse: ");
  az_http_client_send_request(&request_get, &response);

  free(response.begin);
  return exit_code;
}
