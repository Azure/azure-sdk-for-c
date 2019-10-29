// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http_response_read.h>
#include <az_span.h>

#include "./az_test.h"

static void test_http_response_parser() { 
  az_span const response = AZ_STR("HTTP/1.2 404 ");
  az_http_response_state state = az_http_response_state_create(response);
  az_http_response_value value = { 0 };
  az_result const result = az_http_response_state_read(&state, &value);
  TEST_ASSERT(result == AZ_OK);
  TEST_ASSERT(value.kind == AZ_HTTP_RESPONSE_VALUE_STATUS);
  az_http_response_status const status = value.data.status;
  TEST_ASSERT(status.major_version == 1);
  TEST_ASSERT(status.minor_version == 2);
  TEST_ASSERT(status.status_code == 404);
}
