// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <limits.h>
#include <az_test.h>
#include <az_span.h>
#include <az_span_private.h>

#include <_az_cfg.h>

void az_span_append_uint8_NULL_out_span_fails()
{
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_span_append_uint8(buffer, 'a', NULL) == AZ_ERROR_ARG);
}

void az_span_append_uint8_overflow_fails()
{
    uint8_t raw_buffer[2];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_uint8(buffer, 'a', &buffer)));
    TEST_ASSERT(az_succeeded(az_span_append_uint8(buffer, 'b', &buffer)));
    TEST_ASSERT(az_failed(az_span_append_uint8(buffer, 'c', &buffer)));
}

void az_span_append_uint8_succeeds()
{
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));

    TEST_ASSERT(az_succeeded(az_span_append_uint8(buffer, 'a', &buffer)));
    TEST_ASSERT(az_succeeded(az_span_append_uint8(buffer, 'b', &buffer)));
    TEST_ASSERT(az_succeeded(az_span_append_uint8(buffer, 'c', &buffer)));
    TEST_ASSERT(az_span_is_equal(buffer, AZ_SPAN_FROM_STR("abc")));
}

void az_span_append_i32toa_succeeds()
{
    int32_t v = 12345;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_i32toa_negative_succeeds()
{
    int32_t v = -12345;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("-12345")));
}

void az_span_append_i32toa_zero_succeeds()
{
    int32_t v = 0;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_i32toa_max_int_succeeds()
{
    int32_t v = 2147483647;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_i32toa_NULL_span_fails()
{
    int32_t v = 2147483647;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_i32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_i32toa_overflow_fails()
{
    int32_t v = 2147483647;
    uint8_t raw_buffer[4];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_span_append_i32toa(buffer, v, &out_span) == AZ_ERROR_BUFFER_OVERFLOW);
}

void az_span_append_u32toa_succeeds()
{
    uint32_t v = 12345;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("12345")));
}

void az_span_append_u32toa_zero_succeeds()
{
    uint32_t v = 0;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("0")));
}

void az_span_append_u32toa_max_uint_succeeds()
{
    uint32_t v = 4294967295;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("4294967295")));
}

void az_span_append_u32toa_NULL_span_fails()
{
    uint32_t v = 2147483647;
    uint8_t raw_buffer[15];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_succeeded(az_span_append_u32toa(buffer, v, &out_span)));
    TEST_ASSERT(az_span_is_equal(out_span, AZ_SPAN_FROM_STR("2147483647")));
}

void az_span_append_u32toa_overflow_fails()
{
    uint32_t v = 2147483647;
    uint8_t raw_buffer[4];
    az_span buffer = az_span_init(raw_buffer, 0, sizeof(raw_buffer)/sizeof(raw_buffer[0]));
    az_span out_span;

    TEST_ASSERT(az_span_append_u32toa(buffer, v, &out_span) == AZ_ERROR_BUFFER_OVERFLOW);
}

void test_az_span()
{
    az_span_append_uint8_NULL_out_span_fails();
    az_span_append_uint8_overflow_fails();
    az_span_append_uint8_succeeds();

    az_span_append_i32toa_succeeds();
    az_span_append_i32toa_negative_succeeds();
    az_span_append_i32toa_max_int_succeeds();
    az_span_append_i32toa_zero_succeeds();
    az_span_append_i32toa_NULL_span_fails();
    az_span_append_i32toa_overflow_fails();

    az_span_append_u32toa_succeeds();
    az_span_append_u32toa_zero_succeeds();
    az_span_append_u32toa_max_uint_succeeds();
    az_span_append_u32toa_NULL_span_fails();
    az_span_append_u32toa_overflow_fails();
}
