// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

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
void test_json_builder();
void test_json_get_by_pointer();
void test_json_parser();
void test_json_pointer();
void test_json_string();
void test_json_value();

void test_http_response();
void test_http_request();

void test_span();
void test_span_builder_replace();

void test_log();

void test_url_encode();
// end of Defining functions that should be defined in some other .c file

int exit_code = 0;

int main()
{
  test_json_builder();
  test_json_get_by_pointer();
  test_json_parser();
  test_json_pointer();
  test_json_string();
  test_json_value();

  test_http_request();
  test_http_response();

  test_span();
  test_span_builder_replace();

  test_log();

  test_url_encode();

  return exit_code;
}
