// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_context.h>
#include <az_http.h>
#include <az_http_internal.h>
#include <az_http_transport.h>
#include <az_json.h>

#include <az_http_private.h>
#include <az_span.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <az_test.h>

#include <_az_cfg.h>

// Defining functions that should be defined in some other .c file
void test_http_response();
void test_http_request();

void test_az_context();
void test_span();
void test_span_builder_replace();

void test_log();

void test_url_encode();
// end of Defining functions that should be defined in some other .c file

int exit_code = 0;

int main()
{
  test_az_context();

  test_http_request();
  test_http_response();

  test_span();
  test_span_builder_replace();

  test_log();

  test_url_encode();

  return exit_code;
}
