// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include "az_test_definitions.h"

#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <az_json.h>

#include <_az_cfg.h>

/**
 * How to add tests:
 * 1. Add tests definition to az_test_definitions.h. All tests must follow the signature `void
 * test_name(void** state)`.
 * 2. main.c: Add test_names to `CMUnitTest test[]` array at the end of `az_test_definitions.h`
 * 3. Add a .c file with tests' implementation.
 * 4. Uppdate CMakeLists.txt file to make tests include the created .c file
 * Run ctest
 *
 */

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
  /* az_pipeline tests */
  cmocka_unit_test(test_az_pipeline),
  /* az_aad tests */
  cmocka_unit_test(test_az_aad),
  /* az_http_policy tests */
  cmocka_unit_test(test_az_http_policy),

};

int main(void) { return cmocka_run_group_tests_name("az_core", tests, NULL, NULL); }
