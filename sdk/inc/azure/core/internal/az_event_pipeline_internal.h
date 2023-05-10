/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 *
 * @brief Definition of #_az_event_pipeline and related types describing a bi-directional HFSM
 *        pipeline.
 *
 */

#ifndef _az_EVENT_PIPELINE_INTERNAL_H
#define _az_EVENT_PIPELINE_INTERNAL_H

#include <azure/core/az_event.h>
#include <azure/core/az_event_policy.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_hfsm_internal.h>

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Internal definition of a pipeline.
 */
typedef struct
{
  struct
  {
    az_event_policy* outbound_policy;
    az_event_policy* inbound_policy;
    az_platform_mutex mutex;
  } _internal;
} _az_event_pipeline;

/**
 * @brief Initializes a pipeline.
 *
 * @param[out] pipeline The #_az_event_pipeline to initialize.
 * @param[in] outbound The #az_event_policy at the outbound end of the pipeline.
 * @param[in] inbound The #az_event_policy at the inbound end of the pipeline.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK #_az_event_pipeline was successfully initialized.
 * @retval other Initialization failed.
 */
AZ_NODISCARD az_result _az_event_pipeline_init(
    _az_event_pipeline* pipeline,
    az_event_policy* outbound,
    az_event_policy* inbound);

/**
 * @brief Queues an inbound event to the pipeline.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       policy. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] pipeline The #_az_event_pipeline to use for this call.
 * @param[in] event The event being enqueued.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_post_inbound_event(_az_event_pipeline* pipeline, az_event const event);

/**
 * @brief Queues an outbound event to the pipeline.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] pipeline The #_az_event_pipeline to use for this call.
 * @param[in] event The event being enqueued.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_post_outbound_event(_az_event_pipeline* pipeline, az_event const event);

/**
 * @brief Pipeline interval timer interface
 */
typedef struct
{
  _az_platform_timer platform_timer;

  struct
  {
    _az_event_pipeline* pipeline;
  } _internal;
} _az_event_pipeline_timer;

/**
 * @brief Creates an #_az_platform_timer associated with an #_az_event_pipeline.
 * @details When the timer elapses, a TIMEOUT _outbound_ message will be generated. The event.#data
 *          contains a pointer to the original #_az_platform_timer.
 * @param pipeline The pipeline.
 *
 * @param[in, out] out_timer The populated timer structure.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_timer_create(_az_event_pipeline* pipeline, _az_event_pipeline_timer* out_timer);

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_EVENT_PIPELINE_INTERNAL_H
