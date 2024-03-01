// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "az_test_definitions.h"

#include <azure/core/_az_cfg.h>

int main()
{
  int result = 0;

  // every test function returns the number of tests failed, 0 means success (there shouldn't be
  // negative numbers
  result += test_az_base64();
  result += test_az_context();
#if !defined(__APPLE__) && defined(PLATFORM_POSIX) && defined(_az_MOCK_ENABLED)
  result += test_az_event_pipeline();
  result += test_az_hfsm();
  result += test_az_mqtt5_connection();
  result += test_az_mqtt5_topic_parser();
  result += test_az_mqtt5_rpc_server_codec();
  result += test_az_mqtt5_rpc_server();
  result += test_az_mqtt5_rpc_client_codec();
  result += test_az_mqtt5_rpc_client();
  result += test_az_mqtt5_request();
  result += test_az_mqtt5_telemetry_consumer_codec();
  result += test_az_mqtt5_telemetry_consumer();
  result += test_az_mqtt5_telemetry_producer_codec();
  result += test_az_mqtt5_telemetry_producer();
#endif // !defined(__APPLE__) && defined(PLATFORM_POSIX) && defined(_az_MOCK_ENABLED)
  result += test_az_http();
  result += test_az_json();
  result += test_az_logging();
  result += test_az_pipeline();
  result += test_az_policy();
  result += test_az_span();
  result += test_az_url_encode();

  return result;
}
