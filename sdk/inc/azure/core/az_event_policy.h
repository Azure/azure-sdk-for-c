/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 *
 * @brief Definition of #az_event_policy.
 */

#ifndef _az_EVENT_POLICY_H
#define _az_EVENT_POLICY_H

#include <azure/core/az_event.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

// Definition is below.

/// Avoiding a circular dependency.
typedef struct az_event_policy az_event_policy;

/// Avoiding a circular dependency.
typedef az_result (*az_event_policy_handler)(az_event_policy* me, az_event event);

/**
 * @brief The type representing an event-driven policy.
 *
 */
struct az_event_policy
{
  /**
   * @brief The inbound handler for events.
   */
  az_event_policy_handler inbound_handler;

  /**
   * @brief The outbound handler for events.
   */
  az_event_policy_handler outbound_handler;

  /**
   * @brief The policy responsible for handling inbound events.
   */
  az_event_policy* inbound_policy;

  /**
   * @brief The policy responsible for handling outbound events.
   */
  az_event_policy* outbound_policy;
};

/**
 * @brief Sends an inbound event from the current policy.
 *
 * @note This function should be used during pipeline processing. The event is processed on the
 * current thread (stack).
 *
 * @param policy The current policy trying to send the event.
 * @param event The event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result
az_event_policy_send_inbound_event(az_event_policy* policy, az_event const event)
{
  az_event_policy* inbound = policy->inbound_policy;

  _az_PRECONDITION_NOT_NULL(inbound);
  _az_PRECONDITION_NOT_NULL(inbound->inbound_handler);

  az_result ret = inbound->inbound_handler(inbound, event);

  if (az_result_failed(ret))
  {
    // Replace the original event with an error event that is flowed to the application.
    ret = inbound->inbound_handler(
        inbound,
        (az_event){ AZ_HFSM_EVENT_ERROR,
                    &(az_hfsm_event_data_error){
                        .error_type = ret,
                        .sender = policy,
                        .sender_event = event,
                    } });
  }

  return ret;
}

/**
 * @brief Sends an outbound event from the current policy.
 *
 * @note This function should be used during pipeline processing. The event is processed on the
 * current thread (stack).
 *
 * @param policy The current policy trying to send the event.
 * @param event The event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD AZ_INLINE az_result
az_event_policy_send_outbound_event(az_event_policy const* policy, az_event const event)
{
  az_event_policy* outbound = policy->outbound_policy;
  _az_PRECONDITION_NOT_NULL(outbound);
  _az_PRECONDITION_NOT_NULL(outbound->outbound_handler);

  // The error is flowed back to the application.
  return outbound->outbound_handler(outbound, event);
}

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_EVENT_POLICY_H
