// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Configurable constants used by the Azure IoT SDK.
 *
 * @remarks Typically, these constants do not need to be modified but depending on how your
 * application uses an Azure service, they can be adjusted.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_CONFIG_H
#define _az_IOT_CONFIG_H

#include <azure/core/_az_cfg_prefix.h>

enum
{
  /// Maximum Number of Files Handled by this ADU Agent (per step)
  /// This must be no larger than #AZ_IOT_ADU_CLIENT_MAX_TOTAL_FILE_COUNT.
  AZ_IOT_ADU_CLIENT_MAX_FILE_COUNT_PER_STEP = 2,

  /// Maximum Number of Files Handled by this ADU Agent (total files for this deployment)
  AZ_IOT_ADU_CLIENT_MAX_TOTAL_FILE_COUNT = 2,

  /// Maximum Number of Files Handled by this ADU Agent (Steps)
  AZ_IOT_ADU_CLIENT_MAX_INSTRUCTIONS_STEPS = 2,

  /// Maximum Number of Files Handled by this ADU Agent (File Hashes)
  AZ_IOT_ADU_CLIENT_MAX_FILE_HASH_COUNT = 2,

  /// Maximum Number of Custom Device Properties
  AZ_IOT_ADU_CLIENT_MAX_DEVICE_CUSTOM_PROPERTIES = 5
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_CONFIG_H
