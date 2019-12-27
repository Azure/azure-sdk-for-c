// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_span_span.h>
#include <az_str.h>

#include "./az_test.h"

static void test_span_span() {
  {
    az_span src_buffer[4];
    az_span_span_builder span_span_builder
        = az_span_span_builder_create((az_mut_span_span)AZ_SPAN_FROM_ARRAY(src_buffer));

    TEST_ASSERT(span_span_builder.size == 0);

    // append span
    az_span add_this_span = AZ_STR("Something to test");
    az_span add_this_span2 = AZ_STR("test 2");
    az_span add_this_span3 = AZ_STR("test 3");
    az_span add_this_span4 = AZ_STR("test 4");
    az_span add_this_span5 = AZ_STR("test 5 - overflow");

    az_result a1 = az_span_span_builder_append(&span_span_builder, add_this_span);
    az_result a2 = az_span_span_builder_append(&span_span_builder, add_this_span2);
    az_result a3 = az_span_span_builder_append(&span_span_builder, add_this_span3);
    az_result a4 = az_span_span_builder_append(&span_span_builder, add_this_span4);

    TEST_ASSERT(a1 == AZ_OK);
    TEST_ASSERT(a2 == AZ_OK);
    TEST_ASSERT(a3 == AZ_OK);
    TEST_ASSERT(a4 == AZ_OK);

    TEST_ASSERT(span_span_builder.size == 4);

    TEST_ASSERT(az_span_is_equal(src_buffer[0], add_this_span));
    TEST_ASSERT(az_span_is_equal(src_buffer[1], add_this_span2));
    TEST_ASSERT(az_span_is_equal(src_buffer[2], add_this_span3));
    TEST_ASSERT(az_span_is_equal(src_buffer[3], add_this_span4));

    az_result a5 = az_span_span_builder_append(&span_span_builder, add_this_span5);

    TEST_ASSERT(a5 == AZ_ERROR_BUFFER_OVERFLOW);
  }
}
