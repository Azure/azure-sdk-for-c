// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <limits.h>
#include <az_test.h>
#include <az_span.h>
#include <az_span_private.h>

#include <_az_cfg.h>

void az_span_append_byte_NULL_out_span_fails()
{
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_span_append_byte(buffer, 'a', NULL) == AZ_ERROR_ARG);
}

void az_span_append_byte_overflow_fails()
{
    uint8_t raw_buffer[2];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_byte(buffer, 'a', &buffer)));
    TEST_ASSERT(az_succeeded(az_span_append_byte(buffer, 'b', &buffer)));
    TEST_ASSERT(az_failed(az_span_append_byte(buffer, 'c', &buffer)));
}

void az_span_append_byte_succeeds()
{
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_byte(buffer, 'a', &buffer)));
    TEST_ASSERT(az_succeeded(az_span_append_byte(buffer, 'b', &buffer)));
    TEST_ASSERT(az_succeeded(az_span_append_byte(buffer, 'c', &buffer)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("abc")));
}

void az_span_append_int32_succeeds()
{
    int32_t v = 12345;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_int32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_int32_negative_succeeds()
{
    int32_t v = -12345;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_int32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("-12345")));
}

void az_span_append_int32_zero_succeeds()
{
    int32_t v = 0;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_int32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_int32_max_int_succeeds()
{
    int32_t v = 2147483647;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_int32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_int32_NULL_span_fails()
{
    int32_t v = 2147483647;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_int32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_int32_overflow_fails()
{
    int32_t v = 2147483647;
    uint8_t raw_buffer[4];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_span_append_int32(&buffer, v) == AZ_ERROR_BUFFER_OVERFLOW);
}


void az_span_append_uint32_succeeds()
{
    uint32_t v = 12345;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_uint32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_uint32_zero_succeeds()
{
    uint32_t v = 0;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_uint32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_uint32_max_uint_succeeds()
{
    uint32_t v = 4294967295;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_uint32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("4294967295")));
}

void az_span_append_uint32_NULL_span_fails()
{
    uint32_t v = 2147483647;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_uint32(&buffer, v)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_uint32_overflow_fails()
{
    uint32_t v = 2147483647;
    uint8_t raw_buffer[4];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_span_append_uint32(&buffer, v) == AZ_ERROR_BUFFER_OVERFLOW);
}

void test_az_span()
{
    az_span_append_byte_NULL_out_span_fails();
    az_span_append_byte_overflow_fails();
    az_span_append_byte_succeeds();

    az_span_append_int32_succeeds();
    az_span_append_int32_negative_succeeds();
    az_span_append_int32_max_int_succeeds();
    az_span_append_int32_zero_succeeds();
    az_span_append_int32_NULL_span_fails();
    az_span_append_int32_overflow_fails();

    az_span_append_uint32_succeeds();
    az_span_append_uint32_zero_succeeds();
    az_span_append_uint32_max_uint_succeeds();
    az_span_append_uint32_NULL_span_fails();
    az_span_append_uint32_overflow_fails();
}
