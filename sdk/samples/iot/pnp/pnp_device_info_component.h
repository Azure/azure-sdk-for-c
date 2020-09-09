// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef PNP_DEVICE_INFO_COMPONENT_H
#define PNP_DEVICE_INFO_COMPONENT_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

/**
 * @brief Build the reported property payload to send for device info.
 *
 * @param[in] payload An #az_span with sufficient capacity to hold the reported property payload.
 * @param[out] out_payload A pointer to the #az_span containing the reported property payload.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Reported property payload built successfully.
 * @retval other Initialization of #az_json_writer failed or the buffer is too small.
 */
az_result pnp_device_info_build_reported_property(az_span payload, az_span* out_payload);

#endif // PNP_DEVICE_INFO_COMPONENT_H
