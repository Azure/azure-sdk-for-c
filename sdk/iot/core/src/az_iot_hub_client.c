// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_iot_hub_client.h"
#include <az_span.h>

#include <_az_cfg.h>

AZ_NODISCARD az_iot_hub_client_options az_iot_hub_client_options_default()
{
  return (az_iot_hub_client_options){ .module_id = AZ_SPAN_NULL, .user_agent = AZ_SPAN_NULL };
}
