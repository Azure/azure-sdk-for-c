// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef AZ_IOT_SAS_TOKEN_H
#define AZ_IOT_SAS_TOKEN_H

#include <stdlib.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct az_iot_sas_params {
    az_span device_id;
    az_span iothub_fqdn;
    int32_t expiry_time_secs;
    az_span key_name;
} az_iot_sas_params;

/* @brief                         Generates the document to be encrypted and passed to az_iot_sas_token_generate().
 * @param iothub_fqdn[in]         A #az_span containing the FQDN of the Azure IoT Hub.
 * @param device_id[in]           A #az_span containing the FQDN of the Azure IoT Hub.
 * @param expiry_time_secs[in]    The expiry time of the SAS token, in number of seconds since UNIX epoch (1970).
 * @param document[out]           An #az_span where to write the SAS token document to be encrypted.
 * @returns                       An #az_result with the result of the operation.
 * @retval                        #AZ_OK                       If no failures occur. 
 * @retval                        #AZ_ERROR_BUFFER_OVERFLOW    If the buffer provided in #az_iot_sas_params or `document` are insuficient in size.
 * @retval                        #AZ_ERROR_ARG                If `params` is NULL or any of its components are NULL or a NULL #az_span, or 
 *                                                          if `document` is a NULL pointer or a NULL #az_span.
 */
az_result az_iot_sas_token_get_document(az_span iothub_fqdn, az_span device_id, int32_t expiry_time_secs, az_span* document);

/* @brief                        Generates the SAS token based on the parameters and key (encrypted document).
 * @param iothub_fqdn[in]        A #az_span containing the FQDN of the Azure IoT Hub.
 * @param device_id[in]          A #az_span containing the FQDN of the Azure IoT Hub.
 * @param signature[in]          The signature must be generated following this procedure:
 *                               - Obtain the document using az_iot_sas_token_get_document(),
 *                               - Apply HMAC/SHA256 encryption on the document,
 *                                 @note If using a device key as encryption key, it must be Base64 decoded.
 *                               - Base64 encode, then URL encode the encrypted document.
 * @param expiry_time_secs[in]    The expiry time of the SAS token, in number of seconds since UNIX epoch (1970).
 * @param key_name[in]            A #az_span containing the key name to be encoded in the sas token (optional, use az_span_null() if not needed).
 * @param sas_token[out]          An #az_span where to write the SAS token.
 * @returns                       An #az_result with the result of the operation.
 * @retval                        #AZ_OK                       If no failures occur. 
 * @retval                        #AZ_ERROR_BUFFER_OVERFLOW    If the buffer provided in #az_iot_sas_params or `sas_token` are insuficient in size.
 * @retval                        #AZ_ERROR_ARG                If `params` is NULL or any of its components are NULL or a NULL #az_span, or
 *                                                             if `signature` is a NULL #az_span, or
 *                                                             if `document` is a NULL pointer or a NULL #az_span.
 */
az_result az_iot_sas_token_generate(az_span iothub_fqdn, az_span device_id, az_span signature, int32_t expiry_time_secs, az_span key_name, az_span* sas_token);

#include <_az_cfg_suffix.h>

#endif // AZ_IOT_SAS_TOKEN_H
