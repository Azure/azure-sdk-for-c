// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * How to add tests:
 * 1. Add tests definition to az_test_definitions.h. All tests must follow the signature `void
 * test_name(void** state)`.
 * 2. Add test_names to `CMUnitTest test[]` array at the end of `az_test_definitions.h`
 * 3. Add a .c file with tests' implementation.
 * 4. Uppdate CMakeLists.txt file to make tests include the created .c file
 * Run ctest
 *
 */
#include <stddef.h>

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

/* JSON tests */
void test_json_value(void** state);
void test_json_string(void** state);
void test_json_pointer(void** state);
void test_json_parser(void** state);
void test_json_get_by_pointer(void** state);
void test_json_builder(void** state);
void test_json_token_null(void** state);
void test_json_token_boolean(void** state);
void test_json_token_number(void** state);
void test_json_parser_init(void** state);

/* AZ_SPAN tests */
void test_az_span(void** state);
void test_az_span_replace(void** state);
void test_az_span_getters(void** state);

/* AZ_context tests */
void test_az_context(void** state);

/* HTTP Tests */
void test_http_request(void** state);
void test_http_response(void** state);

/* AZ_Log Tests */
void test_az_log(void** state);

/* URL encode tests */
void test_url_encode(void** state);

/* az pipeline tests */
void test_az_pipeline(void** state);

const struct CMUnitTest tests[] = {
  /* URL encode tests */
  cmocka_unit_test(test_url_encode),
  /* AZ_Log Tests */
  cmocka_unit_test(test_az_log),
  /* HTTP Tests */
  cmocka_unit_test(test_http_request),
  cmocka_unit_test(test_http_response),
  /*JSON tests*/
  cmocka_unit_test(test_json_token_null),
  cmocka_unit_test(test_json_parser_init),
  cmocka_unit_test(test_json_token_boolean),
  cmocka_unit_test(test_json_token_number),
  cmocka_unit_test(test_json_value),
  cmocka_unit_test(test_json_string),
  cmocka_unit_test(test_json_pointer),
  cmocka_unit_test(test_json_parser),
  cmocka_unit_test(test_json_get_by_pointer),
  cmocka_unit_test(test_json_builder),
  /*AZ_SPAN tests*/
  cmocka_unit_test(test_az_span),
  cmocka_unit_test(test_az_span_replace),
  cmocka_unit_test(test_az_span_getters),
  /* AZ_context tests */
  cmocka_unit_test(test_az_context),
};
