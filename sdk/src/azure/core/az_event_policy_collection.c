// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_event_policy_collection_internal.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

static az_result _az_event_policy_collection_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_event_policy_collection* policy_collection = (_az_event_policy_collection*)policy;

  // Broadcast to all clients.
  _az_event_client* last = policy_collection->clients;
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

static az_result _az_event_policy_collection_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_event_policy_collection* policy_collection = (_az_event_policy_collection*)policy;

  // Broadcast to all clients.
  _az_event_client* last = policy_collection->clients;
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

AZ_NODISCARD az_result _az_event_policy_collection_init(
    _az_event_policy_collection* policy_collection,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
  policy_collection->policy.outbound_policy = outbound_policy;
  policy_collection->policy.inbound_policy = inbound_policy;
  policy_collection->policy.outbound_handler = _az_event_policy_collection_process_outbound_event;
  policy_collection->policy.inbound_handler = _az_event_policy_collection_process_inbound_event;

  policy_collection->clients = NULL;
  policy_collection->num_clients = 0;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_event_policy_collection_add_client(
    _az_event_policy_collection* policy_collection,
    _az_event_client* client)
{
  _az_PRECONDITION_NOT_NULL(policy_collection);
  _az_PRECONDITION_NOT_NULL(client);

  // Connect the client to the pipeline.
  client->policy->outbound_policy = policy_collection->policy.outbound_policy;
  if (policy_collection->policy.inbound_policy != NULL)
  {
    client->policy->inbound_policy = policy_collection->policy.inbound_policy;
  }

  // The client is added to the end of the list.
  client->next = NULL;

  _az_event_client* last = policy_collection->clients;
  if (last == NULL)
  {
    policy_collection->clients = client;
  }
  else
  {
    while (last->next != NULL)
    {
      last = last->next;
    }

    last->next = client;
  }

  policy_collection->num_clients++;

  return AZ_OK;
}

AZ_NODISCARD az_result _az_event_policy_collection_remove_client(
    _az_event_policy_collection* policy_collection,
    _az_event_client* client)
{
  _az_PRECONDITION_NOT_NULL(policy_collection);
  _az_PRECONDITION_NOT_NULL(client);

  _az_event_client* last = policy_collection->clients;
  if (last == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }
  else if (last == client)
  {
    policy_collection->clients = client->next;
  }
  else
  {
    while (last->next != client)
    {
      last = last->next;
    }
    last->next = client->next;
  }
  policy_collection->num_clients--;

  return AZ_OK;
}
