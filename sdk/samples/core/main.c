/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#include <stdlib.h>
#include <stdio.h>

#include "rpc_client_config.h"

#include <azure/az_core.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt5_rpc.h>
#include <azure/core/az_mqtt5_rpc_client.h>

#include "azure_rpc_client.h"

// Static memory allocation.
static char response_topic_buffer[256];
static char request_topic_buffer[256];
static char subscription_topic_buffer[256];

static azure_rpc_client_t azure_rpc_client;

// static bool az_sdk_log_filter_callback(az_log_classification classification)
// {
//   (void)classification;
//   // Enable all logging.
//   return false;
// }

// static void az_sdk_log_callback(az_log_classification classification, az_span message)
// {
//     (void)classification;
//     (void)message;
// }

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // TODO: verify: if these are not here, there is a segfault.
    // az_log_set_message_callback(az_sdk_log_callback);
    // az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

    azure_rpc_client_config_t azure_rpc_client_config = azure_rpc_client_config_get_default();
    azure_rpc_client_config.connection_options.client_id_buffer = AZ_SPAN_FROM_STR(CLIENT_ID);
    azure_rpc_client_config.connection_options.hostname = AZ_SPAN_FROM_STR(HOSTNAME);
    azure_rpc_client_config.connection_options.username_buffer = AZ_SPAN_FROM_STR(USERNAME);
    azure_rpc_client_config.connection_options.client_certificates[0] =
        (az_mqtt5_x509_client_certificate){ .cert = AZ_SPAN_FROM_STR(CLIENT_CERTIFICATE_PATH), .key = AZ_SPAN_FROM_STR(CLIENT_CERTIFICATE_KEY_PATH) };
    azure_rpc_client_config.model_id = AZ_SPAN_FROM_STR(MODEL_ID);

    if (azure_rpc_platform_initialize() != 0)
    {
        printf("Failed initializing Azure RPC platform\n");
        return __LINE__;
    }

    if (azure_rpc_client_initialize(&azure_rpc_client, azure_rpc_client_config) != 0)
    {
        printf("Failed initializing Azure RPC Client\n");
        return __LINE__;        
    }

    azure_rpc_client_deinitialize(&azure_rpc_client);
    azure_rpc_platform_deinitialize();
    return 0;
}
