// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_pnp_client.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_iot_pnp_client_sas_get_signature(
    az_iot_pnp_client const* client,
    uint32_t token_expiration_epoch_time,
    az_span signature,
    az_span* out_signature)
{
  return az_iot_hub_client_sas_get_signature(
      &(client->_internal.iot_hub_client), token_expiration_epoch_time, signature, out_signature);
}

AZ_NODISCARD az_result az_iot_pnp_client_sas_get_password(
    az_iot_pnp_client const* client,
    uint64_t token_expiration_epoch_time,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    char* mqtt_password,
    size_t mqtt_password_size,
    size_t* out_mqtt_password_length)
{
  return az_iot_hub_client_sas_get_password(
      &(client->_internal.iot_hub_client),
      token_expiration_epoch_time,
      base64_hmac_sha256_signature,
      key_name,
      mqtt_password,
      mqtt_password_size,
      out_mqtt_password_length);
}
