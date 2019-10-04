// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_net.h>
#include <az_str.h>

#include <stdio.h>
#include <string.h>

void println_span(az_const_span const span) {
  for (size_t i = 0; i < span.size; ++i) {
    putchar(span.begin[i]);
  }

  putchar('\n');
}

static uint8_t app_secret_buf[200];
static uint8_t app_body_buf[500];
static uint8_t auth_response_buf[8 * 1024];

int main() {
  AZ_STR_DECL(app_secret, "O2CT[Y:dkTqblml5V/T]ZEi9x1W1zoBW");

  static char const * const app_id = "4317a660-6bfb-4585-9ce9-8f222314879c"; // app_id

  static char const * const auth_uri_str = "https://login.microsoftonline.com/"
                                           "72f988bf-86f1-41af-91ab-2d7cd011db47" // tenant_id
                                           "/oauth2/token";

  az_const_span const auth_uri_span
      = (az_const_span){ .begin = auth_uri_str, .size = strlen(auth_uri_str) };

  size_t const app_secret_buf_size = sizeof(app_secret_buf);
  assert(app_secret_buf_size >= app_secret.size * 2);
  az_span const app_secret_uriencoded
      = (az_span){ .begin = app_secret_buf, .size = app_secret_buf_size };

  az_const_span app_secret_uriencoded_const
      = (az_const_span){ .begin = app_secret_uriencoded.begin };

  az_net_uri_escape(app_secret, app_secret_uriencoded, &app_secret_uriencoded_const);

  //println_span(app_secret_uriencoded_const);

  strcpy(app_body_buf, "grant_type=client_credentials&client_id=");
  strcat(app_body_buf, app_id);
  strcat(app_body_buf, "&client_secret=");

  memcpy(
      app_body_buf + strlen(app_body_buf),
      app_secret_uriencoded_const.begin,
      app_secret_uriencoded_const.size * sizeof(char));

  strcat(app_body_buf, "&resource=https%3A%2F%2Fvault.azure.net");

  az_const_span const app_body_span
      = (az_const_span){ .begin = app_body_buf, .size = strlen(app_body_buf) };

  az_const_span const empty_span = { .size = 0 };

  az_span const auth_response
      = (az_span){ .begin = auth_response_buf, .size = sizeof(auth_response_buf) };

  az_const_span auth_response_const = (az_const_span){ .begin = auth_response.begin };

  az_net_invoke_rest_method(
      AZ_NET_HTTP_METHOD_POST,
      auth_uri_span,
      app_body_span,
      empty_span,
      auth_response,
      &auth_response_const);

  println_span(auth_response_const);

  return 0;
}
