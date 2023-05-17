// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_mqtt_policy.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

static az_result _az_mqtt_policy_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_mqtt_policy* me = (_az_mqtt_policy*)policy;

  switch (event.type)
  {
    case AZ_MQTT_EVENT_CONNECT_REQ:
      _az_RETURN_IF_FAILED(
          az_mqtt_outbound_connect(me->mqtt, me->context, (az_mqtt_connect_data*)event.data));
      break;

    case AZ_MQTT_EVENT_DISCONNECT_REQ:
      _az_PRECONDITION_IS_NULL(event.data);
      _az_RETURN_IF_FAILED(az_mqtt_outbound_disconnect(me->mqtt, me->context));
      break;

    case AZ_MQTT_EVENT_PUB_REQ:
      _az_RETURN_IF_FAILED(
          az_mqtt_outbound_pub(me->mqtt, me->context, (az_mqtt_pub_data*)event.data));
      break;

    case AZ_MQTT_EVENT_SUB_REQ:
      _az_RETURN_IF_FAILED(
          az_mqtt_outbound_sub(me->mqtt, me->context, (az_mqtt_sub_data*)event.data));
      break;

    default:
      az_platform_critical_error();
  }

  return AZ_OK;
}

static az_result _az_mqtt_policy_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
  switch (event.type)
  {
    case AZ_MQTT_EVENT_CONNECT_RSP:
    case AZ_MQTT_EVENT_DISCONNECT_RSP:
    case AZ_MQTT_EVENT_PUB_RECV_IND:
    case AZ_MQTT_EVENT_PUBACK_RSP:
    case AZ_MQTT_EVENT_SUBACK_RSP:
      _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(policy, event));
      break;

    default:
      az_platform_critical_error();
  }

  return AZ_OK;
}

AZ_NODISCARD az_result _az_mqtt_policy_init(
    _az_mqtt_policy* mqtt_policy,
    az_mqtt* mqtt,
    az_context* context,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
  mqtt_policy->mqtt = mqtt;

  mqtt_policy->context = context;
  mqtt_policy->policy.outbound_policy = outbound_policy;
  mqtt_policy->policy.inbound_policy = inbound_policy;

  mqtt_policy->policy.outbound_handler = _az_mqtt_policy_process_outbound_event;
  mqtt_policy->policy.inbound_handler = _az_mqtt_policy_process_inbound_event;

  return AZ_OK;
}
