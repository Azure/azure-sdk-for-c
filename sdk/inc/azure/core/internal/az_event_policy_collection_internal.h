// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Implements a variant of the Hierarchical State Machine Orthogonal Regions pattern.
 * Includes definitions for #_az_event_client and #_az_event_policy_collection.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_EVENT_POLICY_COLLECTION_INTERNAL_H
#define _az_EVENT_POLICY_COLLECTION_INTERNAL_H

#include <azure/core/az_event_policy.h>
#include <azure/core/az_result.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct _az_event_client _az_event_client;
typedef struct _az_event_policy_collection _az_event_policy_collection;

/**
 * @brief An event client contains a policy and a pointer to the next client in a policy collection.
 *
 */
struct _az_event_client
{
  /**
   * @brief The policy.
   *
   */
  az_event_policy* policy;

  /**
   * @brief The next client in the policy collection.
   *
   */
  _az_event_client* next;
};

/**
 * @brief Contains a collection of event clients and is responsible for routing events to them.
 *
 */
struct _az_event_policy_collection
{
  /**
   * @brief Policy which receives events which are routed to the clients in the collection.
   *
   */
  az_event_policy policy;

  /**
   * @brief Pointer to the clients in the collection.
   *
   */
  _az_event_client* clients;
};

/**
 * @brief Initializes a client collection.
 *
 * @param policy_collection The policy that will be used to initialize the orthogonal region.
 * @param outbound_policy The outbound policy for the collection.
 * @param inbound_policy The inbound policy for the collection.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_event_policy_collection_init(
    _az_event_policy_collection* policy_collection,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy);

/**
 * @brief Adds a client to a policy collection. Inserts the client at the end of the list.
 *
 * @param policy_collection The policy collection to add the client to.
 * @param client The client to add to the collection.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_event_policy_collection_add_client(
    _az_event_policy_collection* policy_collection,
    _az_event_client* client);

AZ_NODISCARD az_result _az_event_policy_collection_remove_client(
    _az_event_policy_collection* policy_collection,
    _az_event_client* client);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_EVENT_POLICY_COLLECTION_INTERNAL_H
