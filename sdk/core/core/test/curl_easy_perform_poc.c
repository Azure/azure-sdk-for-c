// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>

#include <az_http_request.h>
#include <az_json_read.h>
#include <az_pair.h>
#include <az_span_builder.h>

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

  return exit_code;
}
