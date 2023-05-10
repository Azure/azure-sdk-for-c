// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

static _az_hfsm az_hfsm_test;

// State handlers
static az_result s_root(az_event_policy* me, az_event event);
static az_result s_01(az_event_policy* me, az_event event);
static az_result s_02(az_event_policy* me, az_event event);
static az_result s_11(az_event_policy* me, az_event event);
static az_result s_12(az_event_policy* me, az_event event);
static az_result s_21(az_event_policy* me, az_event event);
static az_result s_22(az_event_policy* me, az_event event);

// Test HFSM specific events
typedef enum
{
  T_INTERNAL_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 5),
  T_INTERNAL_1 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 6),
  T_INTERNAL_2 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 7),
  T_SUB_R = _az_MAKE_EVENT(_az_FACILITY_HFSM, 8),
  T_SUB_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 9),
  T_SUB_1 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 10),
  T_SUPER_1 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 11),
  T_SUPER_2 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 12),
  T_PEER_0 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 13),
  T_PEER_1 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 14),
  T_PEER_2 = _az_MAKE_EVENT(_az_FACILITY_HFSM, 15)
} test_az_hfsm_event_type;

static az_event t_internal_0_evt = { T_INTERNAL_0, NULL };
static az_event t_internal_1_evt = { T_INTERNAL_1, NULL };
static az_event t_internal_2_evt = { T_INTERNAL_2, NULL };

static az_event t_sub_r_evt = { T_SUB_R, NULL };
static az_event t_sub_0_evt = { T_SUB_0, NULL };
static az_event t_sub_1_evt = { T_SUB_1, NULL };

static az_event t_peer_0_evt = { T_PEER_0, NULL };
static az_event t_peer_1_evt = { T_PEER_1, NULL };
static az_event t_peer_2_evt = { T_PEER_2, NULL };

static az_event t_super_1_evt = { T_SUPER_1, NULL };
static az_event t_super_2_evt = { T_SUPER_2, NULL };

static int ref_root = 0;
static int ref_01 = 0;
static int ref_02 = 0;
static int ref_11 = 0;
static int ref_12 = 0;
static int ref_21 = 0;
static int ref_22 = 0;
static int t_internal_0 = 0;
static int t_internal_1 = 0;
static int t_internal_2 = 0;
static int u_event_root = 0;

// Test HFSM hierarchy structure
static az_event_policy_handler az_hfsm_test_get_parent(az_event_policy_handler child_handler)
{
  az_event_policy_handler parent_handler;

  if ((child_handler == s_root))
  {
    parent_handler = NULL;
  }
  else if ((child_handler == s_01) || (child_handler == s_02))
  {
    parent_handler = s_root;
  }
  else if ((child_handler == s_11) || (child_handler == s_12))
  {
    parent_handler = s_01;
  }
  else if ((child_handler == s_21) || (child_handler == s_22))
  {
    parent_handler = s_11;
  }
  else
  {
    // Unknown state.
    parent_handler = NULL;
  }

  return parent_handler;
}

// TestHFSM/s_root Root Handler
static az_result s_root(az_event_policy* me, az_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_root++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_root--;
      break;

    case T_SUB_R:
      ret = _az_hfsm_transition_substate((_az_hfsm*)me, s_root, s_01);
      break;

    default:
      u_event_root++;
  }

  return ret;
}

// TestHFSM/s_01
static az_result s_01(az_event_policy* me, az_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_01++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_01--;
      break;

    case T_SUB_0:
      ret = _az_hfsm_transition_substate((_az_hfsm*)me, s_01, s_11);
      break;

    case T_PEER_0:
      ret = _az_hfsm_transition_peer((_az_hfsm*)me, s_01, s_02);
      break;

    case T_INTERNAL_0:
      t_internal_0++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/s_02
static az_result s_02(az_event_policy* me, az_event event)
{
  (void)me;

  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_02++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_02--;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/s_01/s_11
static az_result s_11(az_event_policy* me, az_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_11++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_11--;
      break;

    case T_SUB_1:
      ret = _az_hfsm_transition_substate((_az_hfsm*)me, s_11, s_21);
      break;

    case T_PEER_1:
      ret = _az_hfsm_transition_peer((_az_hfsm*)me, s_11, s_12);
      break;

    case T_INTERNAL_1:
      t_internal_1++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/s_01/s_12
static az_result s_12(az_event_policy* me, az_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_12++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_12--;
      break;

    case T_SUPER_1:
      ret = _az_hfsm_transition_superstate((_az_hfsm*)me, s_12, s_01);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/s_01/s_11/s_21
static az_result s_21(az_event_policy* me, az_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_21++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_21--;
      break;

    case T_PEER_2:
      ret = _az_hfsm_transition_peer((_az_hfsm*)me, s_21, s_22);
      break;

    case T_INTERNAL_2:
      t_internal_2++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/s_01/s_11/s_22
static az_result s_22(az_event_policy* me, az_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref_22++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref_22--;
      break;

    case T_SUPER_2:
      ret = _az_hfsm_transition_superstate((_az_hfsm*)me, s_22, s_11);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static void test_az_hfsm_internal_transitions(void** state)
{
  (void)state;

  // Init s_root
  assert_int_equal(
      _az_hfsm_init(&az_hfsm_test, s_root, az_hfsm_test_get_parent, NULL, NULL), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_root);
  assert_true(ref_root == 1);

  // TSubR s_01 -> s_root
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_sub_r_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_01);
  assert_true(ref_root == 1 && ref_01 == 1);

  // TSub0 s_01 -> s_11
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_sub_0_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_11);
  assert_true(ref_root == 1 && ref_01 == 1 && ref_11 == 1);

  // TSub1 s_21
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_sub_1_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_21);
  assert_true(ref_root == 1 && ref_01 == 1 && ref_11 == 1 && ref_21 == 1);

  // TInternal2 s_21
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_2_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_21);
  assert_true(t_internal_2 == 1 && ref_11 == 1 && ref_21 == 1);

  // TInternal1 s_21
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_1_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_21);
  assert_true(t_internal_1 == 1 && ref_21 == 1);

  // TInternal0 s_21
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_0_evt), AZ_OK);
  assert_true(t_internal_0 == 1);

  // TPeer2 s_21 -> s22
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_peer_2_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_22);
  assert_true(ref_root == 1 && ref_01 == 1 && ref_11 == 1 && ref_22 == 1);
  assert_true(ref_21 == 0);

  // TInternal1 s_22
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_1_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_22);
  assert_true(t_internal_1 == 2 && ref_22 == 1);

  // TInternal0 s_22
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_0_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_22);
  assert_true(t_internal_0 == 2 && ref_22 == 1);

  // TSuper2 s_22 -> s_11
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_super_2_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_11);
  assert_true(ref_root == 1 && ref_01 == 1 && ref_11 == 1);
  assert_true(ref_21 == 0 && ref_22 == 0);

  // TInternal1 s_11
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_1_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_11);
  assert_true(t_internal_1 == 3);

  // TInternal0 s_11
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_0_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_11);
  assert_true(t_internal_0 == 3 && ref_11 == 1);

  // TPeer1 s_11 -> s_12
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_peer_1_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_12);
  assert_true(ref_root == 1 && ref_01 == 1 && ref_12 == 1);
  assert_true(ref_21 == 0 && ref_22 == 0 && ref_11 == 0);

  // TInternal0 s_12
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_0_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_12);
  assert_true(t_internal_0 == 4 && ref_12 == 1);

  // TSuper1 s_12 -> s_01
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_super_1_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_01);
  assert_true(ref_root == 1 && ref_01 == 1);
  assert_true(ref_21 == 0 && ref_22 == 0 && ref_11 == 0 && ref_12 == 0);

  // TInternal0 s_01
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_internal_0_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_01);
  assert_true(t_internal_0 == 5 && ref_01 == 1);

  // TPeer0 s_01 -> s_02
  assert_int_equal(_az_hfsm_send_event(&az_hfsm_test, t_peer_0_evt), AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == s_02);
  assert_true(ref_root == 1 && ref_02 == 1);
  assert_true(
      ref_21 == 0 && ref_22 == 0 && ref_11 == 0 && ref_12 == 0 && ref_01 == 0 && ref_02 == 1);

  assert_true(u_event_root == 0);
}

int test_az_hfsm()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_hfsm_internal_transitions),
  };
  return cmocka_run_group_tests_name("az_core_hfsm", tests, NULL, NULL);
}
