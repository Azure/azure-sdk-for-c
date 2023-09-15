#ifndef RPC_CLIENT_CONFIG_H
#define RPC_CLIENT_CONFIG_H

#include <stdlib.h>
#include <azure/az_core.h>

#define CLIENT_ID "mobile-app"

#define MODEL_ID "dtmi:rpc:samples:vehicle;1"

#define USERNAME "mobile-app"

#define HOSTNAME "<hostname>"

#define CLIENT_CERTIFICATE_PATH "<path to cert pem file>"

#define CLIENT_CERTIFICATE_KEY_PATH "<path to cert key file>"

static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span server_client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

#endif // RPC_CLIENT_CONFIG_H
