// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_client_sample pending command functions. You use the RPC
 * client to send commands.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _RPC_CLIENT_PENDING_COMMANDS_H
#define _RPC_CLIENT_PENDING_COMMANDS_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

#ifndef RPC_CLIENT_MAX_PENDING_COMMANDS
#define RPC_CLIENT_MAX_PENDING_COMMANDS 5
#endif

typedef struct pending_command
{
  az_span correlation_id;
  int32_t mid;
  az_context context;
  az_span command_name;
} pending_command;

typedef struct pending_commands_array
{
  int32_t pending_commands_count;

  pending_command commands[RPC_CLIENT_MAX_PENDING_COMMANDS];
} pending_commands_array;

AZ_INLINE az_result pending_commands_array_init(
    pending_commands_array* pending_commands,
    uint8_t correlation_id_buffers[RPC_CLIENT_MAX_PENDING_COMMANDS]
                                  [AZ_MQTT5_RPC_CORRELATION_ID_LENGTH])
{
  pending_commands->pending_commands_count = 0;
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    pending_commands->commands[i].correlation_id
        = az_span_create(correlation_id_buffers[i], AZ_MQTT5_RPC_CORRELATION_ID_LENGTH);
    az_span_fill(pending_commands->commands[i].correlation_id, 0x0);
    pending_commands->commands[i].mid = 0;
    pending_commands->commands[i].command_name = AZ_SPAN_EMPTY;
  }
  return AZ_OK;
}

/**
 * @brief Adds the mid for an already existing command
 */
AZ_INLINE az_result
add_mid_to_command(pending_commands_array* pending_commands, az_span correlation_id, int32_t mid)
{
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_is_content_equal(pending_commands->commands[i].correlation_id, correlation_id))
    {
      pending_commands->commands[i].mid = mid;
      return AZ_OK;
    }
  }
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_INLINE az_result add_command(
    pending_commands_array* pending_commands,
    az_span correlation_id,
    az_span command_name,
    int32_t timout_ms)
{
  az_result ret = AZ_OK;
  if (pending_commands->pending_commands_count >= RPC_CLIENT_MAX_PENDING_COMMANDS)
  {
    printf(LOG_APP_ERROR "Pending Commands Array already has max number of commands.\n");
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  int32_t empty_index = -1;
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_size(pending_commands->commands[i].command_name) == 0)
    {
      empty_index = i;
      break;
    }
  }
  if (empty_index < 0)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  pending_commands->pending_commands_count++;
  az_span_copy(pending_commands->commands[empty_index].correlation_id, correlation_id);
  pending_commands->commands[empty_index].command_name = command_name;

  int64_t clock = 0;
  ret = az_platform_clock_msec(&clock);
  pending_commands->commands[empty_index].context
      = az_context_create_with_expiration(&az_context_application, clock + timout_ms);

  return ret;
}

AZ_INLINE az_result remove_command(pending_commands_array* pending_commands, az_span correlation_id)
{
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_is_content_equal(pending_commands->commands[i].correlation_id, correlation_id))
    {
      az_context_cancel(&pending_commands->commands[i].context);
      az_span_fill(pending_commands->commands[i].correlation_id, 0x0);
      pending_commands->commands[i].mid = 0;
      pending_commands->commands[i].command_name = AZ_SPAN_EMPTY;
      pending_commands->pending_commands_count--;
      return AZ_OK;
    }
  }
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_INLINE bool is_pending_command(pending_commands_array pending_commands, az_span correlation_id)
{
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_is_content_equal(pending_commands.commands[i].correlation_id, correlation_id))
    {
      return true;
    }
  }
  return false;
}

AZ_INLINE pending_command* get_command_with_mid(
    pending_commands_array* pending_commands,
    int32_t mid)
{
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (pending_commands->commands[i].mid == mid)
    {
      return &pending_commands->commands[i];
    }
  }
  return NULL;
}

AZ_INLINE pending_command* get_first_expired_command(pending_commands_array pending_commands)
{
  pending_command* expired_command = NULL;
  int64_t current_time = 0;
  if (az_result_failed(az_platform_clock_msec(&current_time)))
  {
    return NULL;
  }
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_size(pending_commands.commands[i].command_name) > 0
        && az_context_has_expired(&pending_commands.commands[i].context, current_time))
    {
      if (expired_command == NULL
          || az_context_get_expiration(&pending_commands.commands[i].context)
              < az_context_get_expiration(&expired_command->context))
      {
        expired_command = &pending_commands.commands[i];
      }
    }
  }
  return expired_command;
}

#include <azure/core/_az_cfg_suffix.h>

#endif // _RPC_CLIENT_PENDING_COMMANDS_H
