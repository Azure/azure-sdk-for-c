// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_span_private.h"
#include "az_test_definitions.h"
#include <azure/core/az_mqtt5_telemetry.h>
#include <azure/core/az_mqtt5_telemetry_producer_codec.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_mqtt5_topic_parser_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_common.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#define TEST_MODEL_ID "test_model_id"
#define TEST_CLIENT_ID "test_client_id"
#define TEST_TELEMETRY_NAME "test_telemetry_name"
#define TEST_DEFAULT_PRODUCER_TOPIC "services/" TEST_MODEL_ID "/" TEST_CLIENT_ID "/telemetry\0"
#define TEST_CUSTOM_PRODUCER_TOPIC_FORMAT "sender/{senderId}/service/{serviceId}/telemetry/{name}"
#define TEST_CUSTOM_PRODUCER_TOPIC \
  "sender/" TEST_CLIENT_ID "/service/" TEST_MODEL_ID "/telemetry/" TEST_TELEMETRY_NAME "\0"

static void test_az_mqtt5_telemetry_producer_codec_init_no_options_success(void** state)
{
  (void)state;
  az_mqtt5_telemetry_producer_codec producer;
  assert_int_equal(
      az_mqtt5_telemetry_producer_codec_init(
          &producer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_CLIENT_ID), NULL),
      AZ_OK);

  assert_true(
      az_span_is_content_equal(producer._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(
      az_span_is_content_equal(producer._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      producer._internal.options.telemetry_topic_format,
      AZ_SPAN_FROM_STR(AZ_MQTT5_TELEMETRY_DEFAULT_TOPIC_FORMAT)));
}

static void test_az_mqtt5_telemetry_producer_codec_init_options_success(void** state)
{
  (void)state;
  az_mqtt5_telemetry_producer_codec producer;
  az_mqtt5_telemetry_producer_codec_options options
      = az_mqtt5_telemetry_producer_codec_options_default();
  options.telemetry_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_PRODUCER_TOPIC_FORMAT);

  assert_int_equal(
      az_mqtt5_telemetry_producer_codec_init(
          &producer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_CLIENT_ID), &options),
      AZ_OK);

  assert_true(
      az_span_is_content_equal(producer._internal.model_id, AZ_SPAN_FROM_STR(TEST_MODEL_ID)));
  assert_true(
      az_span_is_content_equal(producer._internal.client_id, AZ_SPAN_FROM_STR(TEST_CLIENT_ID)));
  assert_true(az_span_is_content_equal(
      producer._internal.options.telemetry_topic_format,
      AZ_SPAN_FROM_STR(TEST_CUSTOM_PRODUCER_TOPIC_FORMAT)));
}

static void test_az_mqtt5_telemetry_producer_codec_get_publish_topic_default_success(void** state)
{
  (void)state;
  az_span test_default_pub_topic = AZ_SPAN_FROM_STR(TEST_DEFAULT_PRODUCER_TOPIC);
  az_mqtt5_telemetry_producer_codec producer;
  az_result result = az_mqtt5_telemetry_producer_codec_init(
      &producer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_CLIENT_ID), NULL);
  assert_int_equal(result, AZ_OK);

  char test_publish_topic_buffer[az_span_size(test_default_pub_topic)];
  size_t test_publish_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_producer_codec_get_publish_topic(
          &producer,
          AZ_SPAN_EMPTY,
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          &test_publish_topic_buffer_out_size),
      AZ_OK);

  az_span test_pub_topic = az_span_create(
      (uint8_t*)test_publish_topic_buffer, (int32_t)test_publish_topic_buffer_out_size);

  assert_true(az_span_is_content_equal(test_pub_topic, test_default_pub_topic));
}

static void test_az_mqtt5_telemetry_producer_codec_get_publish_topic_custom_success(void** state)
{
  (void)state;
  az_span test_default_pub_topic = AZ_SPAN_FROM_STR(TEST_CUSTOM_PRODUCER_TOPIC);
  az_mqtt5_telemetry_producer_codec producer;
  az_mqtt5_telemetry_producer_codec_options options
      = az_mqtt5_telemetry_producer_codec_options_default();
  options.telemetry_topic_format = AZ_SPAN_FROM_STR(TEST_CUSTOM_PRODUCER_TOPIC_FORMAT);

  az_result result = az_mqtt5_telemetry_producer_codec_init(
      &producer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_CLIENT_ID), &options);
  assert_int_equal(result, AZ_OK);

  char test_publish_topic_buffer[az_span_size(test_default_pub_topic)];
  size_t test_publish_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_producer_codec_get_publish_topic(
          &producer,
          AZ_SPAN_FROM_STR(TEST_TELEMETRY_NAME),
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          &test_publish_topic_buffer_out_size),
      AZ_OK);

  az_span test_pub_topic = az_span_create(
      (uint8_t*)test_publish_topic_buffer, (int32_t)test_publish_topic_buffer_out_size);

  assert_true(az_span_is_content_equal(test_pub_topic, test_default_pub_topic));
}

static void test_az_mqtt5_telemetry_producer_codec_get_publish_topic_buffer_size_failure(
    void** state)
{
  (void)state;
  az_mqtt5_telemetry_producer_codec producer;
  az_result result = az_mqtt5_telemetry_producer_codec_init(
      &producer, AZ_SPAN_FROM_STR(TEST_MODEL_ID), AZ_SPAN_FROM_STR(TEST_CLIENT_ID), NULL);
  assert_int_equal(result, AZ_OK);

  char test_publish_topic_buffer[1];
  size_t test_publish_topic_buffer_out_size = 0;
  assert_int_equal(
      az_mqtt5_telemetry_producer_codec_get_publish_topic(
          &producer,
          AZ_SPAN_EMPTY,
          test_publish_topic_buffer,
          sizeof(test_publish_topic_buffer),
          &test_publish_topic_buffer_out_size),
      AZ_ERROR_NOT_ENOUGH_SPACE);

  assert_int_equal(test_publish_topic_buffer_out_size, 48);
}

int test_az_mqtt5_telemetry_producer_codec()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_codec_init_no_options_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_codec_init_options_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_codec_get_publish_topic_default_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_codec_get_publish_topic_custom_success),
    cmocka_unit_test(test_az_mqtt5_telemetry_producer_codec_get_publish_topic_buffer_size_failure),
  };
  return cmocka_run_group_tests_name("az_core_mqtt5_telemetry_producer_codec", tests, NULL, NULL);
}
