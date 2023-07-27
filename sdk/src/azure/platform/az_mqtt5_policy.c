// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_mqtt5_policy_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

static az_result _az_mqtt5_policy_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_mqtt5_policy* me = (_az_mqtt5_policy*)policy;

  int64_t clock = 0;
  _az_RETURN_IF_FAILED(az_platform_clock_msec(&clock));
  _az_RETURN_IF_FAILED(az_context_has_expired(me->context, clock));

  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_REQ:
      _az_RETURN_IF_FAILED(az_mqtt5_outbound_connect(me->mqtt, (az_mqtt5_connect_data*)event.data));
      break;

    case AZ_MQTT5_EVENT_DISCONNECT_REQ:
      _az_PRECONDITION_IS_NULL(event.data);
      _az_RETURN_IF_FAILED(az_mqtt5_outbound_disconnect(me->mqtt));
      break;

    case AZ_MQTT5_EVENT_PUB_REQ:
      _az_RETURN_IF_FAILED(az_mqtt5_outbound_pub(me->mqtt, (az_mqtt5_pub_data*)event.data));
      break;

    case AZ_MQTT5_EVENT_SUB_REQ:
      _az_RETURN_IF_FAILED(az_mqtt5_outbound_sub(me->mqtt, (az_mqtt5_sub_data*)event.data));
      break;

    default:
      az_platform_critical_error();
  }

  return AZ_OK;
}

static az_result _az_mqtt5_policy_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_MQTT5_EVENT_SUBACK_RSP:
      _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(policy, event));
      break;

    default:
      az_platform_critical_error();
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_mqtt5_policy_init(
    _az_mqtt5_policy* mqtt_policy,
    az_mqtt5* mqtt,
    az_context* context,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
  mqtt_policy->mqtt = mqtt;

  mqtt_policy->context = (context != NULL) ? context : &az_context_application;
  mqtt_policy->policy.outbound_policy = outbound_policy;
  mqtt_policy->policy.inbound_policy = inbound_policy;

  mqtt_policy->policy.outbound_handler = _az_mqtt5_policy_process_outbound_event;
  mqtt_policy->policy.inbound_handler = _az_mqtt5_policy_process_inbound_event;

  return AZ_OK;
}
