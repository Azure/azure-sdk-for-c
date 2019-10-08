// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_net.h>
#include <az_str.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <_az_cfg_warn.h>

void println_span(az_const_span const span) {
  for (size_t i = 0; i < span.size; ++i) {
    putchar(span.begin[i]);
  }

  putchar('\n');
}

static uint8_t app_secret_buf[200];
static uint8_t app_body_buf[500];
static uint8_t auth_response_buf[8 * 1024];

static uint8_t kv_headers_buf[4 * 1024];
static uint8_t kv_response_buf[8 * 1024];

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

  ////////////////////////////////////////////////////////////////////////////////////////////////
  az_net_uri_escape(app_secret, app_secret_uriencoded, &app_secret_uriencoded_const);
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // println_span(app_secret_uriencoded_const);

  strcpy(app_body_buf, "grant_type=client_credentials&client_id=");
  strcat(app_body_buf, app_id);
  strcat(app_body_buf, "&client_secret=");

  memcpy(
      app_body_buf + strlen(app_body_buf),
      app_secret_uriencoded_const.begin,
      app_secret_uriencoded_const.size * sizeof(char));

  strcat(
      app_body_buf, "&resource=https%3A%2F%2Fvault.azure.net"); // not going to bother with another
                                                                // URI escape call for now

  az_const_span const app_body_span
      = (az_const_span){ .begin = app_body_buf, .size = strlen(app_body_buf) };

  az_const_span const empty_span = { .size = 0 };

  az_span const auth_response
      = (az_span){ .begin = auth_response_buf, .size = sizeof(auth_response_buf) };

  az_const_span auth_response_const = (az_const_span){ .begin = auth_response.begin };

  ////////////////////////////////////////////////////////////////////////////////////////////////
  az_net_invoke_rest_method(
      AZ_NET_HTTP_METHOD_POST,
      auth_uri_span,
      app_body_span,
      empty_span,
      auth_response,
      &auth_response_const);
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // println_span(auth_response_const);

  // FIXME: actually JSON-parse auth_response_const to extract access_token'
  const char * const access_token_start = "\"access_token\":\"";
  const char * const access_token_begin
      = strstr(auth_response_const.begin, access_token_start) + strlen(access_token_start);

  assert(access_token_begin != NULL);
  const char * const access_token_end = strchr(access_token_begin, '\"');

  az_const_span const access_token_span
      = (az_const_span){ .begin = access_token_begin,
                         .size = (access_token_end - access_token_begin) / sizeof(char) };

  //println_span(access_token_span);

  AZ_STR_DECL(
      kv_secret_uri,
      "https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0");

  strcpy(kv_headers_buf, "@{'authorization' = 'Bearer ");

  memcpy(
      kv_headers_buf + strlen(kv_headers_buf),
      access_token_span.begin,
      access_token_span.size * sizeof(char));

  strcat(kv_headers_buf, "'}");

  az_const_span const kv_headers_span
      = (az_const_span){ .begin = kv_headers_buf, .size = strlen(kv_headers_buf) };

  az_span const kv_response
      = (az_span){ .begin = kv_response_buf, .size = sizeof(kv_response_buf) };

  az_const_span kv_response_const = (az_const_span){ .begin = kv_response.begin };

  ////////////////////////////////////////////////////////////////////////////////////////////////
  az_net_invoke_rest_method(
      AZ_NET_HTTP_METHOD_GET,
      kv_secret_uri,
      empty_span,
      kv_headers_span,
      kv_response,
      &kv_response_const);
  ////////////////////////////////////////////////////////////////////////////////////////////////

  //println_span(kv_response_const);

  // FIXME: actually JSON-parse auth_response_const to extract access_token'
  const char * const kv_response_start = "\"value\":\"";
  const char * const kv_secret_value_begin
      = strstr(kv_response.begin, kv_response_start) + strlen(kv_response_start);

  assert(kv_secret_value_begin != NULL);
  const char * const kv_secret_value_end = strchr(kv_secret_value_begin, '\"');

  az_const_span const kv_secret_value_span
      = (az_const_span){ .begin = kv_secret_value_begin,
                         .size = (kv_secret_value_end - kv_secret_value_begin) / sizeof(char) };

  println_span(kv_secret_value_span);

  return 0;
}
