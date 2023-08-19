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
  // #ifdef _WIN32
  // static timer_t timer; // placeholder
  // #else
  timer_t timer;
  struct sigevent sev;
  struct itimerspec trigger;
  // #endif
  pending_command* next;
};

/**
 * @brief Start a timer
 */
AZ_INLINE az_result start_timer(void* callback_context,
      int32_t delay_milliseconds,
      void (*timer_callback)(union sigval sv),
      pending_command* command)
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  command->sev.sigev_notify = SIGEV_THREAD;
  command->sev.sigev_notify_function = timer_callback;
  command->sev.sigev_value.sival_ptr = callback_context;
  if (0 != timer_create(CLOCK_REALTIME, &command->sev, &command->timer))
  {
    return AZ_ERROR_ARG;
  }

  // start timer
  command->trigger.it_value.tv_sec = delay_milliseconds / 1000;
  command->trigger.it_value.tv_nsec = (delay_milliseconds % 1000) * 1000000;

  if (0 != timer_settime(command->timer, 0, &command->trigger, NULL))
  {
    printf("failed setting timer\n");
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

/**
 * @brief Stop the timer
 */
AZ_INLINE az_result stop_timer(timer_t timer)
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  if (0 != timer_delete(timer))
  {
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

AZ_INLINE pending_command* add_command(pending_command* pending_commands,
    az_span correlation_id,
    int32_t timout_ms,
    void (*timer_callback)(union sigval sv))
{
  printf("Adding command %s to pending_commands\n", az_span_ptr(correlation_id));
  pending_command* command = (pending_command*)malloc(sizeof(pending_command));
  command->correlation_id = az_span_create(malloc((size_t)az_span_size(correlation_id)), az_span_size(correlation_id));
  az_span_copy(command->correlation_id, correlation_id);
  // command->timer = (timer_t)malloc(sizeof(timer_t));
  // command->sev = (struct sigevent)malloc(sizeof(struct sigevent));
  // command->trigger = (struct itimerspec)malloc(sizeof(struct itimerspec));
  // command->correlation_id = correlation_id;
  command->next = pending_commands;

  start_timer(command, timout_ms, timer_callback, command);
  return command;
}

AZ_INLINE pending_command* remove_command(pending_command* pending_commands, az_span correlation_id)
{
  pending_command* command = pending_commands;
  pending_command* prev = NULL;
  printf("Removing command %s from pending_commands\n", az_span_ptr(correlation_id));
  while (command != NULL)
  {
    if (az_span_is_content_equal(command->correlation_id, correlation_id))
    {
      stop_timer(command->timer);
      free(az_span_ptr(command->correlation_id));
      command->correlation_id = AZ_SPAN_EMPTY;
      if (prev != NULL)
      {
        prev->next = command->next;
        // return pending_commands;
      }
      else
      {
        pending_commands = command->next;
      }
      free(command);
      command = NULL;
      printf("command %s found\n", az_span_ptr(correlation_id));
      return pending_commands;
    }
    prev = command;
    command = command->next;
  }
  // return AZ_ERROR_ITEM_NOT_FOUND;
  printf("command not found\n");
  return NULL;
}


#include <azure/core/_az_cfg_suffix.h>

#endif // _RPC_CLIENT_COMMAND_HASH_TABLE_H
