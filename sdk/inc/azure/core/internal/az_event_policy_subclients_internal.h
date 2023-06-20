// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Implements a variant of the Hierarchical State Machine Orthogonal Regions pattern.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_EVENT_POLICY_SUBCLIENTS_INTERNAL
#define _az_EVENT_POLICY_SUBCLIENTS_INTERNAL

#include <azure/core/az_event_policy.h>
#include <azure/core/az_result.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct _az_event_subclient _az_event_subclient;
typedef struct _az_event_policy_subclients _az_event_policy_subclients;

/**
 * @brief A subclient is a policy that forms part of an orthogonal region.
 *
 */
struct _az_event_subclient
{
  /**
   * @brief The policy.
   *
   */
  az_event_policy* policy;

  /**
   * @brief The next subclient in the orthogonal region.
   *
   */
  _az_event_subclient* next;
};

/**
 * @brief The first subclient in an orthogonal region.
 *
 */
struct _az_event_policy_subclients
{
  /**
   * @brief The policy that will be used to initialize the orthogonal region.
   *
   */
  az_event_policy policy;

  /**
   * @brief Pointer to the next subclient in the orthogonal region.
   *
   */
  _az_event_subclient* subclients;
};

/**
 * @brief Initializes an orthogonal region of subclients.
 *
 * @param policy_subclients The policy that will be used to initialize the orthogonal region.
 * @param outbound_policy The outbound policy for all subclients.
 * @param inbound_policy The inbound policy for all subclients.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_event_policy_subclients_init(
    _az_event_policy_subclients* policy_subclients,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy);

/**
 * @brief Adds a subclient.
 *
 * @param policy_subclients The orthogonal region to which the subclient will be added.
 * @param subclient The subclient to add.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result _az_event_policy_subclients_add_client(
    _az_event_policy_subclients* policy_subclients,
    _az_event_subclient* subclient);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_EVENT_POLICY_SUBCLIENTS_INTERNAL
