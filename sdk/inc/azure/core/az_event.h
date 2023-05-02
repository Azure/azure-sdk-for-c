/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 *
 * @brief Defines events used by the Azure SDK.
 */

#ifndef _az_EVENT_H
#define _az_EVENT_H

#include <azure/core/az_result.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

#define _az_MAKE_EVENT(id, code) ((az_event_type)(((uint32_t)(id) << 16U) | (uint32_t)(code)))

/**
 * @brief The type represents event types.
 */
typedef int32_t az_event_type;

/**
 * @brief The type represents an event.
 */
typedef struct
{
  /**
   * @brief The event type.
   */
  az_event_type type;

  /**
   * @brief The event data.
   */
  void* data;
} az_event;

/**
 * @brief Common event types.
 */
enum az_event_type_generic
{
  /**
   * @brief Generic error event: must use a data field containing a structure derived from
   * #az_hfsm_event_data_error
   */
  AZ_HFSM_EVENT_ERROR = _az_MAKE_EVENT(_az_FACILITY_HFSM, 3),

  /**
   * @brief Generic timeout event: if multiple timers are necessary it's recommended to create
   * separate timeout events.
   */
  AZ_HFSM_EVENT_TIMEOUT = _az_MAKE_EVENT(_az_FACILITY_HFSM, 4),
};

/**
 * @brief The type representing the minimum data required for an #AZ_HFSM_EVENT_ERROR event.
 */
typedef struct
{
  /**
   * @brief The error type as an #az_result.
   */
  az_result error_type;

  /**
   * @brief The sender of the event.
   */
  void* sender;

  /**
   * @brief The event that caused the error.
   */
  az_event sender_event;
} az_hfsm_event_data_error;

#include <azure/core/_az_cfg_suffix.h>

#endif /* _az_EVENT_H */
