// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_pair.h>
#include <az_str.h>

#include "./az_test.h"

static void test_pair_span() {
  {
    az_pair src_buffer[4];
    az_pair_span_builder pair_span_builder
        = az_pair_span_builder_create((az_mut_pair_span)AZ_SPAN_FROM_ARRAY(src_buffer));

    TEST_ASSERT(pair_span_builder.length == 0);

    // append span
    az_pair add_this_pair_span = { .key = AZ_STR("key"), .value = AZ_STR("Something to test") };
    az_pair add_this_pair_span2 = { .key = AZ_STR("key"), .value = AZ_STR("test 2") };
    az_pair add_this_pair_span3 = { .key = AZ_STR("key"), .value = AZ_STR("test 3") };
    az_pair add_this_pair_span4 = { .key = AZ_STR("key"), .value = AZ_STR("test 4") };
    az_pair add_this_pair_span5 = { .key = AZ_STR("key"), .value = AZ_STR("test 5 - overflow") };

    az_result a1 = az_pair_span_builder_append(&pair_span_builder, add_this_pair_span);
    az_result a2 = az_pair_span_builder_append(&pair_span_builder, add_this_pair_span2);
    az_result a3 = az_pair_span_builder_append(&pair_span_builder, add_this_pair_span3);
    az_result a4 = az_pair_span_builder_append(&pair_span_builder, add_this_pair_span4);

    TEST_ASSERT(a1 == AZ_OK);
    TEST_ASSERT(a2 == AZ_OK);
    TEST_ASSERT(a3 == AZ_OK);
    TEST_ASSERT(a4 == AZ_OK);

    TEST_ASSERT(pair_span_builder.length == 4);

    TEST_ASSERT(az_pair_span_is_equal(src_buffer[0], add_this_pair_span));
    TEST_ASSERT(az_pair_span_is_equal(src_buffer[1], add_this_pair_span2));
    TEST_ASSERT(az_pair_span_is_equal(src_buffer[2], add_this_pair_span3));
    TEST_ASSERT(az_pair_span_is_equal(src_buffer[3], add_this_pair_span4));

    az_result a5 = az_pair_span_builder_append(&pair_span_builder, add_this_pair_span5);

    TEST_ASSERT(a5 == AZ_ERROR_BUFFER_OVERFLOW);
  }
}
