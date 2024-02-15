// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

void az_platform_critical_error()
{
  // Should not be called.
  assert_true(false);
}

static _az_event_pipeline az_event_pipeline_test;

// test_policy_outbound <--> test_policy_middle <--> test_policy_inbound.
static _az_hfsm test_policy_outbound;
static _az_hfsm test_policy_middle;
static _az_hfsm test_policy_inbound;

typedef enum
{
  POST_OUTBOUND_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 5),
  POST_INBOUND_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 6),
  SEND_INBOUND_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 7),
  SEND_INBOUND_1 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 8),
  SEND_INBOUND_2 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 9),
  SEND_INBOUND_3 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 10),
  SEND_OUTBOUND_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 11),
  SEND_OUTBOUND_1 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 12),
  SEND_INBOUND_ERROR = _az_MAKE_EVENT(_az_FACILITY_HFSM, 13),
} test_az_event_pipeline_event_type;

static az_event post_outbound_0_evt = { POST_OUTBOUND_0, NULL };
static az_event post_inbound_0_evt = { POST_INBOUND_0, NULL };
static az_event send_inbound_0_evt = { SEND_INBOUND_0, NULL };
static az_event send_inbound_1_evt = { SEND_INBOUND_1, NULL };
static az_event send_inbound_2_evt = { SEND_INBOUND_2, NULL };
static az_event send_inbound_3_evt = { SEND_INBOUND_3, NULL };
static az_event send_outbound_0_evt = { SEND_OUTBOUND_0, NULL };
static az_event send_outbound_1_evt = { SEND_OUTBOUND_1, NULL };
static az_event send_inbound_error_evt = { SEND_INBOUND_ERROR, NULL };

static int ref_policy_01_root = 0;
static int ref_policy_02_root = 0;
static int ref_policy_03_root = 0;
static int timeout_0 = 0;
static int timeout_1 = 0;
static int post_outbound_0 = 0;
static int post_inbound_0 = 0;
static int send_inbound_0 = 0;
static int send_inbound_1 = 0;
static int send_inbound_2 = 0;
static int send_inbound_3 = 0;
static int send_inbound_4 = 0;
static int send_outbound_0 = 0;
static int send_outbound_1 = 0;
static int send_inbound_error_0 = 0;

static az_result policy_01_root(az_event_policy* me, az_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_policy_01_root++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_policy_01_root--;
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      timeout_0++;
      if (timeout_0 > 1)
      {
        // To test timeout failure.
        ret = AZ_ERROR_ARG;
      }
      break;

    case POST_OUTBOUND_0:
      post_outbound_0++;
      break;

    case SEND_INBOUND_0:
      send_inbound_0++;
      ret = az_event_policy_send_inbound_event((az_event_policy*)me, send_inbound_1_evt);
      break;

    case SEND_INBOUND_2:
      send_inbound_2++;
      ret = az_event_policy_send_inbound_event((az_event_policy*)me, send_inbound_3_evt);
      break;

    case SEND_INBOUND_ERROR:
      send_inbound_error_0++;
      ret = az_event_policy_send_inbound_event((az_event_policy*)me, send_inbound_error_evt);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result policy_02_root(az_event_policy* me, az_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_policy_02_root++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_policy_02_root--;
      break;

    case SEND_INBOUND_1:
      send_inbound_1++;
      break;

    case SEND_INBOUND_3:
      send_inbound_3++;
      ret = AZ_ERROR_ARG;
      break;

    case SEND_OUTBOUND_1:
      send_outbound_1++;
      break;

    case SEND_INBOUND_ERROR:
      send_inbound_error_0++;
      ret = AZ_ERROR_CANCELED;
      break;

    case AZ_HFSM_EVENT_ERROR:
      _az_PRECONDITION_NOT_NULL(event.data);
      az_hfsm_event_data_error* test_error = (az_hfsm_event_data_error*)event.data;
      if (test_error->error_type == AZ_ERROR_ARG)
      {
        send_inbound_4++;
      }
      else if (test_error->error_type == AZ_ERROR_CANCELED)
      {
        send_inbound_error_0++;
      }

      else
      {
        ret = AZ_ERROR_NOT_IMPLEMENTED;
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result policy_03_root(az_event_policy* me, az_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_policy_03_root++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_policy_03_root--;
      break;

    case POST_INBOUND_0:
      post_inbound_0++;
      break;

    case SEND_OUTBOUND_0:
      send_outbound_0++;
      ret = az_event_policy_send_outbound_event((az_event_policy*)me, send_outbound_1_evt);
      break;

    case AZ_HFSM_EVENT_ERROR:
      _az_PRECONDITION_NOT_NULL(event.data);
      az_hfsm_event_data_error* test_error = (az_hfsm_event_data_error*)event.data;
      if (test_error->error_type == AZ_ERROR_ARG)
      {
        timeout_1++;
      }
      else
      {
        ret = AZ_ERROR_NOT_IMPLEMENTED;
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_event_policy_handler policy_01_get_parent(az_event_policy_handler child_state)
{
  (void)child_state;

  return NULL;
}

static az_event_policy_handler policy_02_get_parent(az_event_policy_handler child_state)
{
  (void)child_state;

  return NULL;
}

static az_event_policy_handler policy_03_get_parent(az_event_policy_handler child_state)
{
  (void)child_state;

  return NULL;
}

static void test_az_event_pipeline_init(void** state)
{
  (void)state;

  // Init test_policy_outbound policy
  assert_int_equal(
      _az_hfsm_init(
          &test_policy_outbound,
          policy_01_root,
          policy_01_get_parent,
          NULL,
          (az_event_policy*)&test_policy_middle),
      AZ_OK);
  assert_true(test_policy_outbound._internal.current_state == policy_01_root);
  assert_true(ref_policy_01_root == 1);

  // Init test_policy_middle policy
  assert_int_equal(
      _az_hfsm_init(
          &test_policy_middle,
          policy_02_root,
          policy_02_get_parent,
          (az_event_policy*)&test_policy_outbound,
          (az_event_policy*)&test_policy_inbound),
      AZ_OK);
  assert_true(test_policy_middle._internal.current_state == policy_02_root);
  assert_true(ref_policy_02_root == 1);

  // Init test_policy_inbound policy
  assert_int_equal(
      _az_hfsm_init(
          &test_policy_inbound,
          policy_03_root,
          policy_03_get_parent,
          (az_event_policy*)&test_policy_middle,
          NULL),
      AZ_OK);
  assert_true(test_policy_inbound._internal.current_state == policy_03_root);
  assert_true(ref_policy_03_root == 1);

  assert_int_equal(
      _az_event_pipeline_init(
          &az_event_pipeline_test,
          (az_event_policy*)&test_policy_outbound,
          (az_event_policy*)&test_policy_inbound),
      AZ_OK);
}

static void test_az_event_pipeline_post_outbound(void** state)
{
  (void)state;
  post_outbound_0 = 0;

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(&az_event_pipeline_test, post_outbound_0_evt), AZ_OK);
  assert_true(post_outbound_0 == 1);
}

static void test_az_event_pipeline_post_inbound(void** state)
{
  (void)state;
  post_inbound_0 = 0;

  assert_int_equal(
      _az_event_pipeline_post_inbound_event(&az_event_pipeline_test, post_inbound_0_evt), AZ_OK);
  assert_true(post_inbound_0 == 1);
}

static void test_az_event_pipeline_send_inbound(void** state)
{
  (void)state;
  send_inbound_0 = 0;
  send_inbound_1 = 0;

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(&az_event_pipeline_test, send_inbound_0_evt), AZ_OK);
  assert_true(send_inbound_0 == 1);
  assert_true(send_inbound_1 == 1);
}

static void test_az_event_pipeline_send_inbound_failure(void** state)
{
  (void)state;
  send_inbound_2 = 0;
  send_inbound_3 = 0;
  send_inbound_4 = 0;

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(&az_event_pipeline_test, send_inbound_2_evt), AZ_OK);
  assert_true(send_inbound_2 == 1);
  assert_true(send_inbound_3 == 1);
  assert_true(send_inbound_4 == 1);
}

static void test_az_event_pipeline_send_inbound_failure_2(void** state)
{
  (void)state;

  send_inbound_error_0 = 0;

  assert_int_equal(
      _az_event_pipeline_post_outbound_event(&az_event_pipeline_test, send_inbound_error_evt),
      AZ_OK);

  assert_true(send_inbound_error_0 == 3);
}

static void test_az_event_pipeline_send_outbound(void** state)
{
  (void)state;
  send_outbound_0 = 0;
  send_outbound_1 = 0;

  assert_int_equal(
      _az_event_pipeline_post_inbound_event(&az_event_pipeline_test, send_outbound_0_evt), AZ_OK);
  assert_true(send_outbound_0 == 1);
  assert_true(send_outbound_1 == 1);
}

static void test_az_event_pipeline_timer_cb_success(void** state)
{
  (void)state;
  _az_event_pipeline_timer test_timer;
  timeout_0 = 0; // Reset timeout counter

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(_az_event_pipeline_timer_create(&az_event_pipeline_test, &test_timer), AZ_OK);
  assert_int_equal(az_platform_timer_start(&(test_timer.platform_timer), 0), AZ_OK);

  test_timer.platform_timer._internal.platform_timer._internal.callback(
      test_timer.platform_timer._internal.platform_timer._internal.callback_context);

  assert_true(timeout_0 == 1);
}

static void test_az_event_pipeline_timer_cb_failure(void** state)
{
  (void)state;
  _az_event_pipeline_timer test_timer;
  timeout_0 = 1; // Will cause failure on timeout
  timeout_1 = 0;

  will_return(__wrap_az_platform_timer_create, AZ_OK);
  assert_int_equal(_az_event_pipeline_timer_create(&az_event_pipeline_test, &test_timer), AZ_OK);
  assert_int_equal(az_platform_timer_start(&(test_timer.platform_timer), 0), AZ_OK);
  test_timer.platform_timer._internal.platform_timer._internal.callback(
      test_timer.platform_timer._internal.platform_timer._internal.callback_context);
  assert_true(timeout_0 == 2);
  assert_true(timeout_1 == 1);
}

int test_az_event_pipeline()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_event_pipeline_init),
    cmocka_unit_test(test_az_event_pipeline_post_outbound),
    cmocka_unit_test(test_az_event_pipeline_post_inbound),
    cmocka_unit_test(test_az_event_pipeline_send_inbound),
    cmocka_unit_test(test_az_event_pipeline_send_inbound_failure),
    cmocka_unit_test(test_az_event_pipeline_send_inbound_failure_2),
    cmocka_unit_test(test_az_event_pipeline_send_outbound),
    cmocka_unit_test(test_az_event_pipeline_timer_cb_success),
    cmocka_unit_test(test_az_event_pipeline_timer_cb_failure),
  };
  return cmocka_run_group_tests_name("az_core_event_pipeline", tests, NULL, NULL);
}
