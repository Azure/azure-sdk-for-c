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
 * 2. Add test_names to `CMUnitTest test[]` array at the end of `az_test_definitions.h`
 * 3. Add a .c file with tests' implementation.
 * 4. Uppdate CMakeLists.txt file to make tests include the created .c file
 * Run ctest
 *
 */
int main(void) { return cmocka_run_group_tests_name("az_core", tests, NULL, NULL); }
