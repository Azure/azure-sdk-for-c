// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_subclients_internal.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

static az_result _az_event_subclients_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_event_policy_subclients* subclients_policy = (_az_event_policy_subclients*)policy;

  // Broadcast to all orthogonal regions.
  _az_event_subclient* last = subclients_policy->subclients;
  while (last != NULL)
  {
    _az_RETURN_IF_FAILED(last->policy->outbound_handler(last->policy, event));
    last = last->next;
  }

  // TODO_L: Filter events that have been processed;

  // Pass-through to the next outbound policy.
  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(policy, event));

  return AZ_OK;
}

static az_result _az_event_subclients_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_event_policy_subclients* subclients_policy = (_az_event_policy_subclients*)policy;

  // Broadcast to all orthogonal regions.
  _az_event_subclient* last = subclients_policy->subclients;
  while (last != NULL)
  {
    _az_RETURN_IF_FAILED(last->policy->inbound_handler(last->policy, event));
    last = last->next;
  }

  // TODO_L: Filter events that have been processed;

  // Pass-through to the next inbound policy if it exists.
  if (policy->inbound_policy != NULL)
  {
    _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(policy, event));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_event_policy_subclients_init(
    _az_event_policy_subclients* subclients_policy,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
  subclients_policy->policy.outbound_policy = outbound_policy;
  subclients_policy->policy.inbound_policy = inbound_policy;
  subclients_policy->policy.outbound_handler = _az_event_subclients_process_outbound_event;
  subclients_policy->policy.inbound_handler = _az_event_subclients_process_inbound_event;

  subclients_policy->subclients = NULL;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_event_policy_subclients_add_client(
    _az_event_policy_subclients* subclients_policy,
    _az_event_subclient* subclient)
{
  _az_PRECONDITION_NOT_NULL(subclients_policy);
  _az_PRECONDITION_NOT_NULL(subclient);

  // Connect the subclient to the pipeline.
  subclient->policy->outbound_policy = subclients_policy->policy.outbound_policy;
  subclient->policy->inbound_policy = subclients_policy->policy.inbound_policy;

  // The client is added to the end of the list.
  subclient->next = NULL;

  _az_event_subclient* last = subclients_policy->subclients;
  if (last == NULL)
  {
    subclients_policy->subclients = subclient;
  }
  else
  {
    while (last->next != NULL)
    {
      last = last->next;
    }

    last->next = subclient;
  }

  return AZ_OK;
}
