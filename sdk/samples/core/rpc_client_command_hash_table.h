// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_rpc_client_sample. You use the RPC client to send commands.
 *
 * @note The state diagram for this HFSM is in sdk/docs/core/rpc_client.puml
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _RPC_CLIENT_COMMAND_HASH_TABLE_H
#define _RPC_CLIENT_COMMAND_HASH_TABLE_H

#include <azure/core/az_mqtt5_connection.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct pending_command pending_command;

struct pending_command
{
  az_span correlation_id;
  az_context context;
  pending_command* next;
};

AZ_INLINE pending_command* add_command(pending_command* pending_commands,
    az_span correlation_id,
    int32_t timout_ms)
{
  printf("Adding command %s to pending_commands\n", az_span_ptr(correlation_id));
  pending_command* command = (pending_command*)malloc(sizeof(pending_command));
  command->correlation_id = az_span_create(malloc((size_t)az_span_size(correlation_id)), az_span_size(correlation_id));
  az_span_copy(command->correlation_id, correlation_id);
  command->next = pending_commands;

  int64_t clock = 0;
  az_result ret = az_platform_clock_msec(&clock);
  (void)ret;
  command->context = az_context_create_with_expiration(&az_context_application, clock + timout_ms);

  return command;
}

AZ_INLINE az_result remove_command(pending_command** pending_commands, az_span correlation_id)
{
  pending_command* command = *pending_commands;
  pending_command* prev = NULL;
  // printf("Removing command %s from pending_commands\n", az_span_ptr(correlation_id));
  while (command != NULL)
  {
    if (az_span_is_content_equal(command->correlation_id, correlation_id))
    {
      // printf("command %s found\n", az_span_ptr(correlation_id));
      az_context_cancel(&command->context);
      free(az_span_ptr(command->correlation_id));
      command->correlation_id = AZ_SPAN_EMPTY;
      if (prev != NULL)
      {
        prev->next = command->next;
      }
      else
      {
        *pending_commands = command->next;
      }
      free(command);
      command = NULL;
      
      return AZ_OK;
    }
    prev = command;
    command = command->next;
  }
  // printf("command not found\n");
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_INLINE bool is_pending_command(pending_command* pending_commands, az_span correlation_id)
{
  pending_command* command = pending_commands;
  while (command != NULL)
  {
    if (az_span_is_content_equal(command->correlation_id, correlation_id))
    {
      return true;
    }
    command = command->next;
  }
  return false;
}

AZ_INLINE pending_command* get_first_expired_command(pending_command* pending_commands)
{
  pending_command* command = pending_commands;
  pending_command* expired_command = NULL;
  int64_t clock = 0;
  az_result ret = az_platform_clock_msec(&clock);
  (void) ret;
  while (command != NULL)
  {
    if (az_context_has_expired(&command->context, clock))
    {
      // printf("command %s expired\n", az_span_ptr(command->correlation_id));
      if (expired_command == NULL || az_context_get_expiration(&command->context) < az_context_get_expiration(&expired_command->context))
      {
        expired_command = command;
      }
    }
    command = command->next;
  }
  return expired_command;
}


#include <azure/core/_az_cfg_suffix.h>

#endif // _RPC_CLIENT_COMMAND_HASH_TABLE_H
