/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 *
 * @brief Hierarchical Finite State Machine (HFSM) implementation.
 *
 * @details This implementation is _not_ providing complete HFSM functionality. The following
 *          constraints must be made by the developer for their state machines:
 *          1. A single top level state must exist.
 *          2. Transitions can only be made to sub-states, peer-states and super-states.
 *          3. The initial state is always the top-level state. Transitions must be made by the
 *             application if an inner state must be reached during initialization.
 */

#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <stddef.h>
#include <stdint.h>

#include <azure/core/_az_cfg.h>

const az_event _az_hfsm_event_entry = { AZ_HFSM_EVENT_ENTRY, NULL };
const az_event _az_hfsm_event_exit = { AZ_HFSM_EVENT_EXIT, NULL };

static az_result _az_hfsm_event_handler(az_event_policy* h, az_event event)
{
  return _az_hfsm_send_event((_az_hfsm*)h, event);
}

AZ_NODISCARD az_result _az_hfsm_init(
    _az_hfsm* h,
    az_event_policy_handler root_state,
    _az_hfsm_get_parent get_parent_func,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(root_state);
  _az_PRECONDITION_NOT_NULL(get_parent_func);
  h->_internal.current_state = root_state;
  h->_internal.get_parent_func = get_parent_func;

  h->policy.inbound_handler = _az_hfsm_event_handler;
  h->policy.outbound_handler = _az_hfsm_event_handler;

  h->policy.outbound_policy = outbound_policy;
  h->policy.inbound_policy = inbound_policy;

  az_result ret = h->_internal.current_state((az_event_policy*)h, _az_hfsm_event_entry);

  // The HFSM should never return this result.
  _az_PRECONDITION(ret != AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE);
  return ret;
}

static AZ_NODISCARD az_result
_az_hfsm_recursive_exit(_az_hfsm* h, az_event_policy_handler source_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);

  // Super-state handler making a transition must exit all substates:
  while (source_state != h->_internal.current_state)
  {
    // A top-level state is mandatory to ensure an Least Common Ancestor exists.
    _az_PRECONDITION_NOT_NULL(h->_internal.current_state);

    _az_RETURN_IF_FAILED(h->_internal.current_state((az_event_policy*)h, _az_hfsm_event_exit));
    az_event_policy_handler super_state = h->_internal.get_parent_func(h->_internal.current_state);
    _az_PRECONDITION_NOT_NULL(super_state);

    h->_internal.current_state = super_state;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_hfsm_transition_peer(
    _az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // Super-state handler making a transition must exit all inner states:
  _az_RETURN_IF_FAILED(_az_hfsm_recursive_exit(h, source_state));
  _az_PRECONDITION(h->_internal.current_state == source_state);

  // Exit the source state.
  _az_RETURN_IF_FAILED(h->_internal.current_state((az_event_policy*)h, _az_hfsm_event_exit));

  // Enter the destination state:
  _az_RETURN_IF_FAILED(destination_state((az_event_policy*)h, _az_hfsm_event_entry));
  h->_internal.current_state = destination_state;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_hfsm_transition_substate(
    _az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // Super-state handler making a transition must exit all inner states:
  _az_RETURN_IF_FAILED(_az_hfsm_recursive_exit(h, source_state));
  _az_PRECONDITION(h->_internal.current_state == source_state);

  // Transitions to sub-states will not exit the super-state:
  _az_RETURN_IF_FAILED(destination_state((az_event_policy*)h, _az_hfsm_event_entry));
  h->_internal.current_state = destination_state;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_hfsm_transition_superstate(
    _az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // Super-state handler making a transition must exit all inner states:
  _az_RETURN_IF_FAILED(_az_hfsm_recursive_exit(h, source_state));
  _az_PRECONDITION(h->_internal.current_state == source_state);

  // Transitions to super states will exit the substate but not enter the superstate again:
  _az_RETURN_IF_FAILED(h->_internal.current_state((az_event_policy*)h, _az_hfsm_event_exit));
  h->_internal.current_state = destination_state;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_hfsm_send_event(_az_hfsm* h, az_event event)
{
  _az_PRECONDITION_NOT_NULL(h);
  az_result ret;

  az_event_policy_handler current = h->_internal.current_state;
  _az_PRECONDITION_NOT_NULL(current);
  ret = current((az_event_policy*)h, event);

  while (ret == AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE)
  {
    az_event_policy_handler super = h->_internal.get_parent_func(current);

    // Top-level state must handle _all_ events.
    _az_PRECONDITION_NOT_NULL(super);
    current = super;
    ret = current((az_event_policy*)h, event);
  }

  return ret;
}
