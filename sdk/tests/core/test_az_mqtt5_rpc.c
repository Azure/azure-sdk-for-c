// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_span_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_COMMAND_NAME "test_command_name"
#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_client_id"
#define TEST_SERVER_ID "test_server_id"

static void test_az_rpc_get_topic_from_format_success(void** state)
{
  (void)state;

  char test_topic_buffer[256];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("something/{serviceId}/another/{executorId}/{name}/__for_{invokerId}"),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length),
      AZ_SPAN_FROM_STR("something/" TEST_MODEL_ID "/another/" TEST_SERVER_ID "/" TEST_COMMAND_NAME
                       "/__for_" TEST_CLIENT_ID "\0")));
}

static void test_az_rpc_get_topic_from_format_no_customization_success(void** state)
{
  (void)state;

  char test_topic_buffer[10];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("no_subs"),
          AZ_SPAN_EMPTY,
          AZ_SPAN_EMPTY,
          AZ_SPAN_EMPTY,
          AZ_SPAN_EMPTY,
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length), AZ_SPAN_FROM_STR("no_subs\0")));
}

static void test_az_rpc_get_topic_from_format_long_success(void** state)
{
  (void)state;

  char test_topic_buffer[490];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR(
              "{name}/AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/"
              "{serviceId}/BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB/"
              "{executorId}/CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC/"
              "{invokerId}"),
          AZ_SPAN_FROM_STR("111111111111111111111111111111111111111111111111111111111111111111111"),
          AZ_SPAN_FROM_STR("222222222222222222222222222222222222222222222222222222222222222222222"),
          AZ_SPAN_FROM_STR("333333333333333333333333333333333333333333333333333333333333333333333"),
          AZ_SPAN_FROM_STR("444444444444444444444444444444444444444444444444444444444444444444444"),
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length),
      AZ_SPAN_FROM_STR("444444444444444444444444444444444444444444444444444444444444444444444/"
                       "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/"
                       "111111111111111111111111111111111111111111111111111111111111111111111/"
                       "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB/"
                       "222222222222222222222222222222222222222222222222222222222222222222222/"
                       "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC/"
                       "333333333333333333333333333333333333333333333333333333333333333333333\0")));
}

static void test_az_rpc_get_topic_from_format_exact_buffer_size_success(void** state)
{
  (void)state;

  char test_topic_buffer[39];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("type/{invokerId}/{executorId}/{name}/{serviceId}"),
          AZ_SPAN_FROM_STR("1"),
          AZ_SPAN_FROM_STR("2"),
          AZ_SPAN_FROM_STR("3"),
          AZ_SPAN_FROM_STR("4"),
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length), AZ_SPAN_FROM_STR("type/3/2/4/1\0")));
}

static void test_az_rpc_get_topic_from_format_no_model_id_success(void** state)
{
  (void)state;

  char test_topic_buffer[256];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("something/another/{executorId}/{name}/__for_{invokerId}"),
          AZ_SPAN_EMPTY,
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length),
      AZ_SPAN_FROM_STR("something/another/" TEST_SERVER_ID "/" TEST_COMMAND_NAME
                       "/__for_" TEST_CLIENT_ID "\0")));
}

static void test_az_rpc_get_topic_from_format_no_server_id_success(void** state)
{
  (void)state;

  char test_topic_buffer[256];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("something/{serviceId}/another/{name}/__for_{invokerId}"),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_EMPTY,
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length),
      AZ_SPAN_FROM_STR("something/" TEST_MODEL_ID "/another/" TEST_COMMAND_NAME
                       "/__for_" TEST_CLIENT_ID "\0")));
}

static void test_az_rpc_get_topic_from_format_no_client_id_success(void** state)
{
  (void)state;

  char test_topic_buffer[256];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("something/{serviceId}/another/{executorId}/{name}/"),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_EMPTY,
          AZ_SPAN_FROM_STR(TEST_COMMAND_NAME),
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length),
      AZ_SPAN_FROM_STR("something/" TEST_MODEL_ID "/another/" TEST_SERVER_ID "/" TEST_COMMAND_NAME
                       "/\0")));
}

static void test_az_rpc_get_topic_from_format_no_command_name_success(void** state)
{
  (void)state;

  char test_topic_buffer[256];
  az_span test_topic = AZ_SPAN_FROM_BUFFER(test_topic_buffer);
  int32_t test_topic_length;

  assert_int_equal(
      az_rpc_get_topic_from_format(
          AZ_SPAN_FROM_STR("something/{serviceId}/another/{executorId}/__for_{invokerId}"),
          AZ_SPAN_FROM_STR(TEST_MODEL_ID),
          AZ_SPAN_FROM_STR(TEST_SERVER_ID),
          AZ_SPAN_FROM_STR(TEST_CLIENT_ID),
          AZ_SPAN_EMPTY,
          test_topic,
          &test_topic_length),
      AZ_OK);

  assert_true(az_span_is_content_equal(
      az_span_slice(test_topic, 0, test_topic_length),
      AZ_SPAN_FROM_STR("something/" TEST_MODEL_ID "/another/" TEST_SERVER_ID
                       "/__for_" TEST_CLIENT_ID "\0")));
}

static void test_az_rpc_topic_match_sub(void** state)
{
  (void)state;

  /* Topic Subscription Match Testing */

  /* Testing Empty */

  // Sub: AZ_SPAN_EMPTY Topic: "A" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_EMPTY, AZ_SPAN_FROM_STR("A")));

  // Sub: "A" Topic: AZ_SPAN_EMPTY (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_EMPTY));

  // Sub: "" Topic: "A" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR(""), AZ_SPAN_FROM_STR("A")));

  // Sub: "A" Topic: "" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_FROM_STR("")));

  /* Testing Illegal Topic */

  // Sub: "$" Topic: "A" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("$"), AZ_SPAN_FROM_STR("A")));

  // Sub: "A" Topic: "#" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_FROM_STR("#")));

  // Sub: "A" Topic: "+" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_FROM_STR("+")));

  // Sub: "A/" Topic: "A/#" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/"), AZ_SPAN_FROM_STR("A/#")));

  // Sub: "A/" Topic: "A/+" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/"), AZ_SPAN_FROM_STR("A/+")));

  // Sub: "A" Topic: "A\n" (Illegal) 
  //assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_FROM_STR("A\0")));

  /* Testing Illegal Subscription */

  // Sub: "A" Topic: "$" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_FROM_STR("$")));

  // Sub: "#A/B" Topic: "A/B" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("#A/B"), AZ_SPAN_FROM_STR("A/B")));

  // Sub: "+A/B" Topic: "A/B" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+A/B"), AZ_SPAN_FROM_STR("A/B")));

  // Sub: "A/#B" Topic: "A/B" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/#B"), AZ_SPAN_FROM_STR("A/B")));

  // Sub: "A/+B" Topic: "A/B" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/+B"), AZ_SPAN_FROM_STR("A/B")));

  // Sub: "A/B#" Topic: "A/B" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/B#"), AZ_SPAN_FROM_STR("A/B")));

  // Sub: "A/B+" Topic: "A/B" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/B+"), AZ_SPAN_FROM_STR("A/B")));

  // Sub: "A\n" Topic: "A" (Illegal)
  //assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A\0"), AZ_SPAN_FROM_STR("A")));

  // Sub: "A\n" Topic: "A\n" (Illegal)
  //assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A\0"), AZ_SPAN_FROM_STR("A\0")));

  /* Testing Illegal Multi-Level Wildcard */

  // Sub: "#/" Topic: "A/" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("#/"), AZ_SPAN_FROM_STR("A/")));

  // Sub: "A/#/C" Topic: "A/B/C" (Illegal)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/#/C"), AZ_SPAN_FROM_STR("A/B/C")));

  /* Matching Multi-Level Wildcard */

  // Sub: "#" Topic: "ABC/DEF/GHI" (True)
  assert_true(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("#"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "#" Topic: "/" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("#"), AZ_SPAN_FROM_STR("/")));

  // Sub: "ABC/#" Topic: "ABC/DEF/GHI" (True)
  assert_true(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/#"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "ABC/#" Topic: "ABC" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/#"), AZ_SPAN_FROM_STR("ABC")));

  // Sub: "ABC/#" Topic: "ABC/" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/#"), AZ_SPAN_FROM_STR("ABC/")));

  // Sub: "ABC/+/#" Topic: "ABC/DEF" (True)
  assert_true(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/+/#"), AZ_SPAN_FROM_STR("ABC/DEF")));

  /* Matching Single-Level Wildcard */

  // Sub: "+" Topic: "/" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+"), AZ_SPAN_FROM_STR("/")));

  // Sub: "+" Topic: "ABC" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+"), AZ_SPAN_FROM_STR("ABC")));

  // Sub: "+/" Topic: "ABC" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+/"), AZ_SPAN_FROM_STR("ABC")));

  // Sub: "+/+" Topic: "/ABC" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+/+"), AZ_SPAN_FROM_STR("/ABC")));

  // Sub: "/+" Topic: "/ABC" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("/+"), AZ_SPAN_FROM_STR("/ABC")));

  // Sub: "+" Topic: "/ABC" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+"), AZ_SPAN_FROM_STR("/ABC")));

  // Sub: "ABC/+" Topic: "ABC/DEF/GHI" (False)
  assert_false(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/+"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "ABC/DEF/+" Topic: "ABC/DEF/" (True)
  assert_true(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/DEF/+"), AZ_SPAN_FROM_STR("ABC/DEF/")));

  // Sub: "ABC/DEF/+" Topic: "ABC/DEF" (False)
  assert_false(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/DEF/+"), AZ_SPAN_FROM_STR("ABC/DEF")));

  // Sub: "ABC/+/GHI" Topic: "ABC/DEF/GHI" (True)
  assert_true(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/+/GHI"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "+/+/+" Topic: "ABC/DEF/GHI" (True)
  assert_true(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+/+/+"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "+/DEF/+" Topic: "ABC/DEF/GHI" (True)
  assert_true(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("+/DEF/+"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "A/+/" Topic: "A//" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/+"), AZ_SPAN_FROM_STR("A//")));

  // Sub: "/+/+/+" Topic: "///" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("/+/+/+"), AZ_SPAN_FROM_STR("///")));

  // Sub: "ABC/DEF/+/" Topic: "ABC/DEF/GHI/" (True)
  assert_true(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF/+/"), AZ_SPAN_FROM_STR("ABC/DEF/GHI/")));

  // Sub: "ABC/DEF/+" Topic: "ABC/DEF/GHI" (True)
  assert_true(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF/+"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  /* Matching Non-Wildcard */

  // Sub: " " Topic: " " (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR(" "), AZ_SPAN_FROM_STR(" ")));

  // Sub: "/" Topic: "/" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("/"), AZ_SPAN_FROM_STR("/")));

  // Sub: "ABC" Topic: "/" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A"), AZ_SPAN_FROM_STR("/")));

  // Sub: "/ABC" Topic: "/" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("/A"), AZ_SPAN_FROM_STR("/")));

  // Sub: "ABC" Topic: "ABC" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC"), AZ_SPAN_FROM_STR("ABC")));

  // Sub: "AbC" Topic: "ABC" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC"), AZ_SPAN_FROM_STR("AbC")));

  // Sub: /ABC Topic: "ABC" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("/ABC"), AZ_SPAN_FROM_STR("ABC")));

  // Sub: "ABC/" Topic: "ABC" (False)
  assert_false(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/"), AZ_SPAN_FROM_STR("ABC")));

  // Sub: "A/B/C" Topic: "A/B/C" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("A/B/C"), AZ_SPAN_FROM_STR("A/B/C")));

  // Sub: "ABC/DEF/GHI" Topic: "ABC/DEF/GHI" (True)
  assert_true(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF/GHI"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "ABC/DEF/GHI/" Topic: "ABC/DEF/GHI" (False)
  assert_false(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF/GHI/"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "ABC/DEF/GHI" Topic: "ABC/DEF/GHI/" (False)
  assert_false(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF/GHI"), AZ_SPAN_FROM_STR("ABC/DEF/GHI/")));

  // Sub: "/ABC/DEF/GHI" Topic: "ABC/DEF/GHI" (False)
  assert_false(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("/ABC/DEF/GHI"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "ABC/DEF/GHI" Topic: "/ABC/DEF/GHI" (False)
  assert_false(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF/GHI"), AZ_SPAN_FROM_STR("/ABC/DEF/GHI")));

  // Sub: "ABC/DEF" Topic: "ABC/DEF/GHI" (False)
  assert_false(
      _az_span_topic_matches_filter(AZ_SPAN_FROM_STR("ABC/DEF"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "ABC/DEF//GHI" Topic: "ABC/DEF/GHI" (False)
  assert_false(_az_span_topic_matches_filter(
      AZ_SPAN_FROM_STR("ABC/DEF//GHI"), AZ_SPAN_FROM_STR("ABC/DEF/GHI")));

  // Sub: "/////" Topic: "/////" (True)
  assert_true(_az_span_topic_matches_filter(AZ_SPAN_FROM_STR("/////"), AZ_SPAN_FROM_STR("/////")));
}

int test_az_mqtt5_rpc()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_rpc_get_topic_from_format_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_no_customization_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_long_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_exact_buffer_size_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_no_model_id_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_no_server_id_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_no_client_id_success),
    cmocka_unit_test(test_az_rpc_get_topic_from_format_no_command_name_success),
    cmocka_unit_test(test_az_rpc_topic_match_sub),

  };
  return cmocka_run_group_tests_name("az_core_mqtt5_rpc", tests, NULL, NULL);
}
