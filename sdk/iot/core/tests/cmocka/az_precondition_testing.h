// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_PRECONDITION_TESTING_H
#define _az_PRECONDITION_TESTING_H

#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

// This block defines the resources needed to verify precondition checking.
// Macro enable_precondition_check_tests() (no semi-colon at end) shall be invoked in the unit test module right after includes.
// Macro setup_precondition_check_tests() shall be invoked in the unit test module entry function, before any test is executed.
// Macro assert_precondition_checked(func) shall be used in each test to assert if a precondition is verified.
// If a precondition is not verified within a function the result will be either:
// - A crash in the test, as the the target function will continue to execute with an invalid argument, or
// - An assert failure indicating a precondition was not tested.
// Notice that:
// - If assert_precondition_checked(func) is used, the module must include <setjmp.h>;
// - If a function has two precondition checks and both are supposed to fail on a given test, assert_precondition_checked(func)
//   is unable to distinguish which precondition has failed first. Testing precondition checking separately is advised.
// - Tests using assert_precondition_checked(func) currently must not be run in parallel (!).

#define enable_precondition_check_tests() \
static jmp_buf g_precond_test_jmp_buf; \
static unsigned int precondition_test_count = 0; \
static void az_precondition_test_failed_fn() \
{ \
  precondition_test_count++; \
  longjmp(g_precond_test_jmp_buf, 0); \
}

#define setup_precondition_check_tests() \
  az_precondition_failed_set_callback(az_precondition_test_failed_fn);

#define assert_precondition_checked(fn) \
  precondition_test_count = 0; \
  (void)setjmp(g_precond_test_jmp_buf); \
  if (precondition_test_count == 0) { \
    fn; \
  } \
  assert_int_equal(1, precondition_test_count);


#endif // _az_PRECONDITION_TESTING_H
