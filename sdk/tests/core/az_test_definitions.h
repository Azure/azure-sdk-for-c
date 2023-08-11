// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

int test_az_base64();
int test_az_context();
#if !defined(__APPLE__) && defined(PLATFORM_POSIX)
int test_az_event_pipeline();
int test_az_hfsm();
#if defined(_az_MOCK_ENABLED)
int test_az_mqtt5_connection();
int test_az_mqtt5_rpc_server();
#endif // _az_MOCK_ENABLED
#endif // !defined(__APPLE__) && defined(PLATFORM_POSIX)
int test_az_http();
int test_az_json();
int test_az_logging();
int test_az_pipeline();
int test_az_policy();
int test_az_span();
int test_az_url_encode();
