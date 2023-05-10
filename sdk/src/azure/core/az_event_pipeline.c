// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/*
 * @file
 *
 * @brief Event pipeline implementation.
 *
 */

#include <azure/core/az_event_policy.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_event_pipeline_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result _az_event_pipeline_init(
    _az_event_pipeline* pipeline,
    az_event_policy* outbound,
    az_event_policy* inbound)
{
  pipeline->_internal.outbound_policy = outbound;
  pipeline->_internal.inbound_policy = inbound;
  return az_platform_mutex_init(&pipeline->_internal.mutex);
}

AZ_NODISCARD az_result
_az_event_pipeline_post_outbound_event(_az_event_pipeline* pipeline, az_event const event)
{
  az_result ret;

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));

  ret = pipeline->_internal.outbound_policy->outbound_handler(
      pipeline->_internal.outbound_policy, event);

  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));

  return ret;
}

AZ_NODISCARD az_result
_az_event_pipeline_post_inbound_event(_az_event_pipeline* pipeline, az_event const event)
{
  az_result ret;

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));

  ret = pipeline->_internal.inbound_policy->inbound_handler(
      pipeline->_internal.inbound_policy, event);

  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));

  return ret;
}

static void _az_event_pipeline_timer_callback(void* callback_context)
{
  _az_event_pipeline_timer* timer = (_az_event_pipeline_timer*)callback_context;
  az_event timer_event = (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT, .data = timer };

  az_result ret = _az_event_pipeline_post_outbound_event(timer->_internal.pipeline, timer_event);

  if (az_result_failed(ret))
  {
    ret = _az_event_pipeline_post_inbound_event(
        timer->_internal.pipeline,
        (az_event){ .type = AZ_HFSM_EVENT_ERROR,
                    .data = &(az_hfsm_event_data_error){
                        .error_type = ret,
                        .sender_event = timer_event,
                        .sender = timer->_internal.pipeline->_internal.outbound_policy,
                    } });
  }

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

AZ_NODISCARD az_result
_az_event_pipeline_timer_create(_az_event_pipeline* pipeline, _az_event_pipeline_timer* out_timer)
{
  out_timer->_internal.pipeline = pipeline;

  return az_platform_timer_create(
      &out_timer->platform_timer, _az_event_pipeline_timer_callback, out_timer);
}
