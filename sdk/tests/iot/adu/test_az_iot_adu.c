// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_adu.h"
#include <az_test_log.h>
#include <az_test_precondition.h>
#include <az_test_span.h>
#include <azure/core/az_log.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_adu_client.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#define TEST_SPAN_BUFFER_SIZE 1024
#define TEST_ADU_DEVICE_MANUFACTURER "Contoso"
#define TEST_ADU_DEVICE_MODEL "Foobar"
#define TEST_AZ_IOT_ADU_CLIENT_AGENT_VERSION AZ_IOT_ADU_CLIENT_AGENT_VERSION
#define TEST_ADU_DEVICE_VERSION "1.0"

static uint8_t expected_agent_state_payload[]
    = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\"deviceProperties\":{\"manufacturer\":"
      "\"Contoso\",\"model\":\"Foobar\",\"interfaceId\":\"dtmi:azure:iot:deviceUpdate;1\","
      "\"aduVer\":\"DU;agent/"
      "0.8.0-rc1-public-preview\"},\"compatPropertyNames\":\"manufacturer,model\",\"state\":0,"
      "\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\","
      "\\\"version\\\":\\\"1.0\\\"}\"}}}";
static uint8_t expected_agent_state_long_payload[]
    = "{\"deviceUpdate\":{\"__t\":\"c\",\"agent\":{\"deviceProperties\":{\"manufacturer\":"
      "\"Contoso\",\"model\":\"Foobar\",\"interfaceId\":\"dtmi:azure:iot:deviceUpdate;1\","
      "\"aduVer\":\"DU;agent/"
      "0.8.0-rc1-public-preview\"},\"compatPropertyNames\":\"manufacturer,model\","
      "\"lastInstallResult\":{\"resultCode\":0,\"extendedResultCode\":1234,\"resultDetails\":"
      "\"Ok\",\"step_0\":{\"resultCode\":0,\"extendedResultCode\":1234,\"resultDetails\":\"Ok\"}},"
      "\"state\":0,\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\"},"
      "\"installedUpdateId\":\"{\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\","
      "\\\"version\\\":\\\"1.0\\\"}\"}}}";

az_iot_adu_client_device_properties adu_device_properties
    = { .manufacturer = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MANUFACTURER),
        .model = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MODEL),
        .adu_version = AZ_SPAN_LITERAL_FROM_STR(TEST_AZ_IOT_ADU_CLIENT_AGENT_VERSION),
        .delivery_optimization_agent_version = AZ_SPAN_LITERAL_EMPTY,
        .update_id = { .provider = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MANUFACTURER),
                       .name = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_MODEL),
                       .version = AZ_SPAN_LITERAL_FROM_STR(TEST_ADU_DEVICE_VERSION) } };

static uint8_t send_response_valid_payload[]
    = "{\"deviceUpdate\":{\"__t\":\"c\",\"service\":{\"ac\":200,\"av\":1,\"value\":{}}}}";
static uint8_t scratch_buffer[8000];
static uint8_t device_update_subcomponent_name[] = "deviceUpdate";
static int32_t result_code = 0;
static int32_t extended_result_code = 1234;
static az_span result_details = AZ_SPAN_LITERAL_FROM_STR("Ok");

/*Request Values */
static uint8_t adu_request_payload[]
    = "{\"service\":{\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-892a-c822549b6f38\"},"
      "\"updateManifest\":\"{\\\"manifestVersion\\\":\\\"4\\\",\\\"updateId\\\":{\\\"provider\\\":"
      "\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.1\\\"},"
      "\\\"compatibility\\\":[{\\\"deviceManufacturer\\\":\\\"Contoso\\\",\\\"deviceModel\\\":"
      "\\\"Foobar\\\"}],\\\"instructions\\\":{\\\"steps\\\":[{\\\"handler\\\":\\\"microsoft/"
      "swupdate:1\\\",\\\"files\\\":[\\\"f2f4a804ca17afbae\\\"],\\\"handlerProperties\\\":{"
      "\\\"installedCriteria\\\":\\\"1.0\\\"}}]},\\\"files\\\":{\\\"f2f4a804ca17afbae\\\":{"
      "\\\"fileName\\\":\\\"iot-middleware-sample-adu-v1.1\\\",\\\"sizeInBytes\\\":844976,"
      "\\\"hashes\\\":{\\\"sha256\\\":\\\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/"
      "WBi0=\\\"}}},\\\"createdDateTime\\\":\\\"2022-07-07T03:02:48.8449038Z\\\"}\","
      "\"updateManifestSignature\":"
      "\"eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdV"
      "VpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRl"
      "ltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0"
      "VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6Qkhkam"
      "wzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdG"
      "tWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUF"
      "N6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWV"
      "VSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbE"
      "pTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTn"
      "JNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU"
      "9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTW"
      "tGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVV"
      "JWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2"
      "RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYT"
      "NkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1Rm"
      "piVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSan"
      "ZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZF"
      "NtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm"
      "5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISk"
      "VoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0."
      "eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9."
      "eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-"
      "Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_"
      "BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-"
      "G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_"
      "4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-"
      "43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-"
      "q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR\",\"fileUrls\":{"
      "\"f2f4a804ca17afbae\":\"http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/"
      "westus2/contoso-adu-instance--contoso-adu/67c8d2ef5148403391bed74f51a28597/"
      "iot-middleware-sample-adu-v1.1\"}}}";
static uint8_t adu_request_payload_reverse_order[]
    = "{\"service\":{\"updateManifest\":\"{\\\"manifestVersion\\\":\\\"4\\\",\\\"updateId\\\":{"
      "\\\"provider\\\":\\\"Contoso\\\",\\\"name\\\":\\\"Foobar\\\",\\\"version\\\":\\\"1.1\\\"},"
      "\\\"compatibility\\\":[{\\\"deviceManufacturer\\\":\\\"Contoso\\\",\\\"deviceModel\\\":"
      "\\\"Foobar\\\"}],\\\"instructions\\\":{\\\"steps\\\":[{\\\"handler\\\":\\\"microsoft/"
      "swupdate:1\\\",\\\"files\\\":[\\\"f2f4a804ca17afbae\\\"],\\\"handlerProperties\\\":{"
      "\\\"installedCriteria\\\":\\\"1.0\\\"}}]},\\\"files\\\":{\\\"f2f4a804ca17afbae\\\":{"
      "\\\"fileName\\\":\\\"iot-middleware-sample-adu-v1.1\\\",\\\"sizeInBytes\\\":844976,"
      "\\\"hashes\\\":{\\\"sha256\\\":\\\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/"
      "WBi0=\\\"}}},\\\"createdDateTime\\\":\\\"2022-07-07T03:02:48.8449038Z\\\"}\","
      "\"updateManifestSignature\":"
      "\"eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdV"
      "VpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRl"
      "ltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0"
      "VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6Qkhkam"
      "wzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdG"
      "tWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUF"
      "N6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWV"
      "VSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbE"
      "pTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTn"
      "JNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU"
      "9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTW"
      "tGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVV"
      "JWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2"
      "RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYT"
      "NkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1Rm"
      "piVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSan"
      "ZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZF"
      "NtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm"
      "5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISk"
      "VoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0."
      "eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9."
      "eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-"
      "Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_"
      "BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-"
      "G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_"
      "4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-"
      "43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-"
      "q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR\",\"fileUrls\":{"
      "\"f2f4a804ca17afbae\":\"http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/"
      "westus2/contoso-adu-instance--contoso-adu/67c8d2ef5148403391bed74f51a28597/"
      "iot-middleware-sample-adu-v1.1\"},\"workflow\":{\"action\":3,\"id\":\"51552a54-765e-419f-"
      "892a-c822549b6f38\"}}}";
static uint8_t adu_request_manifest[]
    = "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Foobar\","
      "\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":\"Contoso\",\"deviceModel\":"
      "\"Foobar\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/"
      "swupdate:1\",\"files\":[\"f2f4a804ca17afbae\"],\"handlerProperties\":{\"installedCriteria\":"
      "\"1.0\"}}]},\"files\":{\"f2f4a804ca17afbae\":{\"fileName\":\"iot-middleware-sample-adu-v1."
      "1\",\"sizeInBytes\":844976,\"hashes\":{\"sha256\":\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/"
      "WBi0=\"}}},\"createdDateTime\":\"2022-07-07T03:02:48.8449038Z\"}";
static uint8_t adu_request_manifest_reverse_order[]
    = "{\"createdDateTime\":\"2022-07-07T03:02:48.8449038Z\",\"files\":{\"f2f4a804ca17afbae\":{"
      "\"fileName\":\"iot-middleware-sample-adu-v1.1\",\"sizeInBytes\":844976,\"hashes\":{"
      "\"sha256\":\"xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/"
      "WBi0=\"}}},\"instructions\":{\"steps\":[{\"handler\":\"microsoft/"
      "swupdate:1\",\"files\":[\"f2f4a804ca17afbae\"],\"handlerProperties\":{\"installedCriteria\":"
      "\"1.0\"}}]},\"compatibility\":[{\"deviceManufacturer\":\"Contoso\",\"deviceModel\":"
      "\"Foobar\"}],\"updateId\":{\"provider\":\"Contoso\",\"name\":\"Foobar\",\"version\":\"1.1\"}"
      ",\"manifestVersion\":\"4\"}";
static uint32_t workflow_action = 3;
static uint8_t workflow_id[] = "51552a54-765e-419f-892a-c822549b6f38";
static uint8_t manifest_version[] = "4";
static uint8_t update_id_provider[] = "Contoso";
static uint8_t update_id_name[] = "Foobar";
static uint8_t update_id_version[] = "1.1";
static uint8_t instructions_steps_handler[] = "microsoft/swupdate:1";
static uint8_t instructions_steps_file[] = "f2f4a804ca17afbae";
static uint8_t instructions_steps_handler_properties_install_criteria[] = "1.0";
static uint8_t files_id[] = "f2f4a804ca17afbae";
static uint8_t files_filename[] = "iot-middleware-sample-adu-v1.1";
static uint32_t files_size_in_bytes = 844976;
static uint8_t files_hash_id[] = "sha256";
static uint8_t files_hashes_sha[] = "xsoCnYAMkZZ7m9RL9Vyg9jKfFehCNxyuPFaJVM/WBi0=";
static uint8_t created_date_time[] = "2022-07-07T03:02:48.8449038Z";
static uint8_t valid_signature[]
    = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVp"
      "SjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRllt"
      "Tm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VW"
      "amVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6Qkhkamwz"
      "VjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtW"
      "bnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6"
      "azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVS"
      "VmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpT"
      "V2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJN"
      "bGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9X"
      "cFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtG"
      "d04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJW"
      "TGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RI"
      "Z1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNk"
      "Zk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1Rmpi"
      "VGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZG"
      "bjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNt"
      "R2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5s"
      "ZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVo"
      "QmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0."
      "eyJzaGEyNTYiOiJiUlkrcis0MzdsYTV5d2hIeDdqVHhlVVRkeDdJdXQyQkNlcVpoQys5bmFNPSJ9."
      "eYoBoq9EOiCebTJAMhRh9DARC69F3C4Qsia86no9YbMJzwKt-rH88Va4dL59uNTlPNBQid4u0RlXSUTuma_v-"
      "Sf4hyw70tCskwru5Fp41k9Ve3YSkulUKzctEhaNUJ9tUSA11Tz9HwJHOAEA1-S_dXWR_yuxabk9G_"
      "BiucsuKhoI0Bas4e1ydQE2jXZNdVVibrFSqxvuVZrxHKVhwm-"
      "G9RYHjZcoSgmQ58vWyaC2l8K8ZqnlQWmuLur0CZFQlanUVxDocJUtu1MnB2ER6emMRD_"
      "4Azup2K4apq9E1EfYBbXxOZ0N5jaSr-2xg8NVSow5NqNSaYYY43wy_NIUefRlbSYu5zOrSWtuIwRdsO-"
      "43Eo8b9vuJj1Qty9ee6xz1gdUNHnUdnM6dHEplZK0GZznsxRviFXt7yv8bVLd32Z7QDtFh3s17xlKulBZxWP-"
      "q96r92RoUTov2M3ynPZSDmc6Mz7-r8ioO5VHO5pAPCH-tF5zsqzipPJKmBMaf5gYk8wR";
static uint8_t file_url[] = "http://contoso-adu-instance--contoso-adu.b.nlu.dl.adu.microsoft.com/"
                            "westus2/contoso-adu-instance--contoso-adu/"
                            "67c8d2ef5148403391bed74f51a28597/iot-middleware-sample-adu-v1.1";

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

// PRECONDITION TESTS

static void test_az_iot_adu_client_init_NULL_client_fail(void** state)
{
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_init(NULL, NULL));
}

static void test_az_iot_adu_is_component_device_update_NULL_client_fail(void** state)
{
  (void)state;

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_is_component_device_update(
      NULL,
      az_span_create(
          device_update_subcomponent_name, sizeof(device_update_subcomponent_name) - 1)));
}

static void test_az_iot_adu_client_get_agent_state_payload_NULL_client_fail(void** state)
{
  (void)state;

  az_json_writer jw;

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_get_agent_state_payload(
      NULL, &adu_device_properties, AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE, NULL, NULL, &jw));
}

static void test_az_iot_adu_client_get_agent_state_payload_NULL_device_info_fail(void** state)
{
  (void)state;

  az_iot_adu_client client;
  az_json_writer jw;

  assert_int_equal(az_iot_adu_client_init(&client, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_get_agent_state_payload(
      &client, NULL, AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE, NULL, NULL, &jw));
}

static void test_az_iot_adu_client_get_service_properties_response_NULL_client_fail(void** state)
{
  (void)state;

  az_json_writer jw;

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_get_service_properties_response(
      NULL, 1, AZ_IOT_ADU_CLIENT_REQUEST_ACCEPTED, &jw));
}

static void test_az_iot_adu_client_get_service_properties_response_NULL_json_writer_fail(
    void** state)
{
  (void)state;

  az_iot_adu_client adu_client;

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_get_service_properties_response(
      &adu_client, 1, AZ_IOT_ADU_CLIENT_REQUEST_ACCEPTED, NULL));
}

static void test_az_iot_adu_client_parse_service_properties_NULL_client_fail(void** state)
{
  (void)state;

  az_json_reader reader;
  az_iot_adu_client_update_request request;
  az_span remainder;

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_parse_service_properties(
      NULL, &reader, az_span_create(scratch_buffer, sizeof(scratch_buffer)), &request, &remainder));
}

static void test_az_iot_adu_client_parse_service_properties_NULL_reader_fail(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_iot_adu_client_update_request request;
  az_span remainder;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_parse_service_properties(
      &adu_client,
      NULL,
      az_span_create(scratch_buffer, sizeof(scratch_buffer)),
      &request,
      &remainder));
}

static void test_az_iot_adu_client_parse_service_properties_empty_buffer_fail(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;
  az_iot_adu_client_update_request request;
  az_span remainder;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_parse_service_properties(
      &adu_client, &reader, AZ_SPAN_EMPTY, &request, &remainder));
}

static void test_az_iot_adu_client_parse_service_properties_NULL_request_fail(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;
  az_span remainder;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_parse_service_properties(
      &adu_client,
      &reader,
      az_span_create(scratch_buffer, sizeof(scratch_buffer)),
      NULL,
      &remainder));
}

static void test_az_iot_adu_client_parse_update_manifest_NULL_client_fail(void** state)
{
  (void)state;

  az_json_reader reader;
  az_iot_adu_client_update_manifest update_manifest;

  ASSERT_PRECONDITION_CHECKED(
      az_iot_adu_client_parse_update_manifest(NULL, &reader, &update_manifest));
}

static void test_az_iot_adu_client_parse_update_manifest_NULL_reader_fail(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_iot_adu_client_update_manifest update_manifest;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(
      az_iot_adu_client_parse_update_manifest(&adu_client, NULL, &update_manifest));
}

static void test_az_iot_adu_client_parse_update_manifest_NULL_update_manifest_fail(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  ASSERT_PRECONDITION_CHECKED(az_iot_adu_client_parse_update_manifest(&adu_client, &reader, NULL));
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_iot_adu_client_init_succeed(void** state)
{
  (void)state;

  az_iot_adu_client client;

  assert_int_equal(az_iot_adu_client_init(&client, NULL), AZ_OK);
}

static void test_az_iot_adu_is_component_device_update_succeed(void** state)
{
  (void)state;

  az_iot_adu_client client;

  assert_int_equal(az_iot_adu_client_init(&client, NULL), AZ_OK);

  assert_true(az_iot_adu_client_is_component_device_update(
      &client,
      az_span_create(
          device_update_subcomponent_name, sizeof(device_update_subcomponent_name) - 1)));
}

static void test_az_iot_adu_client_get_agent_state_payload_succeed(void** state)
{
  (void)state;

  az_iot_adu_client client;
  az_json_writer jw;
  uint8_t payload_buffer[TEST_SPAN_BUFFER_SIZE];

  assert_int_equal(az_iot_adu_client_init(&client, NULL), AZ_OK);

  assert_int_equal(
      az_json_writer_init(&jw, az_span_create(payload_buffer, sizeof(payload_buffer)), NULL),
      AZ_OK);

  assert_int_equal(
      az_iot_adu_client_get_agent_state_payload(
          &client, &adu_device_properties, AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE, NULL, NULL, &jw),
      AZ_OK);

  assert_memory_equal(
      payload_buffer, expected_agent_state_payload, sizeof(expected_agent_state_payload));
}

static void test_az_iot_adu_client_get_agent_state_long_payload_succeed(void** state)
{
  (void)state;

  az_iot_adu_client client;
  az_iot_adu_client_update_request request;
  az_iot_adu_client_install_result install_result;
  az_json_writer jw;
  az_json_reader jr;
  uint8_t payload_buffer[TEST_SPAN_BUFFER_SIZE];
  az_span remainder;

  install_result.extended_result_code = extended_result_code;
  install_result.result_code = result_code;
  install_result.result_details = result_details;
  install_result.step_results_count = 1;
  install_result.step_results[0].result_code = result_code;
  install_result.step_results[0].extended_result_code = extended_result_code;
  install_result.step_results[0].result_details = result_details;

  assert_int_equal(az_iot_adu_client_init(&client, NULL), AZ_OK);

  assert_int_equal(
      az_json_reader_init(
          &jr, az_span_create(adu_request_payload, sizeof(adu_request_payload) - 1), NULL),
      AZ_OK);

  // parse_service_properties requires that the reader be placed on the "service" prop name
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);
  assert_int_equal(az_json_reader_next_token(&jr), AZ_OK);

  assert_int_equal(
      az_iot_adu_client_parse_service_properties(
          &client,
          &jr,
          az_span_create(scratch_buffer, sizeof(scratch_buffer)),
          &request,
          &remainder),
      AZ_OK);

  assert_int_equal(
      az_json_writer_init(&jw, az_span_create(payload_buffer, sizeof(payload_buffer)), NULL),
      AZ_OK);

  assert_int_equal(
      az_iot_adu_client_get_agent_state_payload(
          &client,
          &adu_device_properties,
          AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE,
          &request.workflow,
          &install_result,
          &jw),
      AZ_OK);

  assert_memory_equal(
      payload_buffer, expected_agent_state_long_payload, sizeof(expected_agent_state_long_payload));
}

static void test_az_iot_adu_client_get_service_properties_response_succeed(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_writer jw;

  az_span payload = az_span_create(scratch_buffer, sizeof(scratch_buffer));

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  assert_int_equal(az_json_writer_init(&jw, payload, NULL), AZ_OK);

  assert_int_equal(
      az_iot_adu_client_get_service_properties_response(
          &adu_client, 1, AZ_IOT_ADU_CLIENT_REQUEST_ACCEPTED, &jw),
      AZ_OK);

  payload = az_json_writer_get_bytes_used_in_destination(&jw);

  assert_memory_equal(
      az_span_ptr(payload), send_response_valid_payload, sizeof(send_response_valid_payload) - 1);
  assert_int_equal(az_span_size(payload), sizeof(send_response_valid_payload) - 1);
}

static void test_az_iot_adu_client_parse_service_properties_succeed(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;
  az_iot_adu_client_update_request request;
  az_span remainder;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  assert_int_equal(
      az_json_reader_init(
          &reader, az_span_create(adu_request_payload, sizeof(adu_request_payload) - 1), NULL),
      AZ_OK);

  // parse_service_properties requires that the reader be placed on the "service" prop name
  assert_int_equal(az_json_reader_next_token(&reader), AZ_OK);
  assert_int_equal(az_json_reader_next_token(&reader), AZ_OK);

  assert_int_equal(
      az_iot_adu_client_parse_service_properties(
          &adu_client,
          &reader,
          az_span_create(scratch_buffer, sizeof(scratch_buffer)),
          &request,
          &remainder),
      AZ_OK);

  // Workflow
  assert_int_equal(request.workflow.action, workflow_action);
  assert_memory_equal(az_span_ptr(request.workflow.id), workflow_id, sizeof(workflow_id) - 1);

  // Update Manifest
  assert_memory_equal(
      az_span_ptr(request.update_manifest), adu_request_manifest, sizeof(adu_request_manifest) - 1);
  assert_int_equal(az_span_size(request.update_manifest), sizeof(adu_request_manifest) - 1);

  // Signature
  assert_memory_equal(
      az_span_ptr(request.update_manifest_signature), valid_signature, sizeof(valid_signature) - 1);
  assert_int_equal(az_span_size(request.update_manifest_signature), sizeof(valid_signature) - 1);

  // File URLs
  assert_memory_equal(az_span_ptr(request.file_urls[0].id), files_id, sizeof(files_id) - 1);
  assert_int_equal(az_span_size(request.file_urls[0].id), sizeof(files_id) - 1);
  assert_memory_equal(az_span_ptr(request.file_urls[0].url), file_url, sizeof(file_url) - 1);
  assert_int_equal(az_span_size(request.file_urls[0].url), sizeof(file_url) - 1);
}

static void test_az_iot_adu_client_parse_service_properties_payload_reverse_order_succeed(
    void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;
  az_iot_adu_client_update_request request;
  az_span remainder;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  assert_int_equal(
      az_json_reader_init(
          &reader,
          az_span_create(
              adu_request_payload_reverse_order, sizeof(adu_request_payload_reverse_order) - 1),
          NULL),
      AZ_OK);

  // parse_service_properties requires that the reader be placed on the "service" prop name
  assert_int_equal(az_json_reader_next_token(&reader), AZ_OK);
  assert_int_equal(az_json_reader_next_token(&reader), AZ_OK);

  assert_int_equal(
      az_iot_adu_client_parse_service_properties(
          &adu_client,
          &reader,
          az_span_create(scratch_buffer, sizeof(scratch_buffer)),
          &request,
          &remainder),
      AZ_OK);

  // Workflow
  assert_int_equal(request.workflow.action, workflow_action);
  assert_memory_equal(az_span_ptr(request.workflow.id), workflow_id, sizeof(workflow_id) - 1);

  // Update Manifest
  assert_memory_equal(
      az_span_ptr(request.update_manifest), adu_request_manifest, sizeof(adu_request_manifest) - 1);
  assert_int_equal(az_span_size(request.update_manifest), sizeof(adu_request_manifest) - 1);

  // Signature
  assert_memory_equal(
      az_span_ptr(request.update_manifest_signature), valid_signature, sizeof(valid_signature) - 1);
  assert_int_equal(az_span_size(request.update_manifest_signature), sizeof(valid_signature) - 1);

  // File URLs
  assert_memory_equal(az_span_ptr(request.file_urls[0].id), files_id, sizeof(files_id) - 1);
  assert_int_equal(az_span_size(request.file_urls[0].id), sizeof(files_id) - 1);
  assert_memory_equal(az_span_ptr(request.file_urls[0].url), file_url, sizeof(file_url) - 1);
  assert_int_equal(az_span_size(request.file_urls[0].url), sizeof(file_url) - 1);
}

static void test_az_iot_adu_client_parse_update_manifest_succeed(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;
  az_iot_adu_client_update_manifest update_manifest;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  assert_int_equal(
      az_json_reader_init(
          &reader, az_span_create(adu_request_manifest, sizeof(adu_request_manifest) - 1), NULL),
      AZ_OK);

  assert_int_equal(
      az_iot_adu_client_parse_update_manifest(&adu_client, &reader, &update_manifest), AZ_OK);

  assert_memory_equal(
      az_span_ptr(update_manifest.manifest_version),
      manifest_version,
      sizeof(manifest_version) - 1);
  assert_int_equal(az_span_size(update_manifest.manifest_version), sizeof(manifest_version) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.update_id.provider),
      update_id_provider,
      sizeof(update_id_provider) - 1);
  assert_int_equal(
      az_span_size(update_manifest.update_id.provider), sizeof(update_id_provider) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.update_id.name), update_id_name, sizeof(update_id_name) - 1);
  assert_int_equal(az_span_size(update_manifest.update_id.name), sizeof(update_id_name) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.update_id.version),
      update_id_version,
      sizeof(update_id_version) - 1);
  assert_int_equal(az_span_size(update_manifest.update_id.version), sizeof(update_id_version) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.instructions.steps[0].handler),
      instructions_steps_handler,
      sizeof(instructions_steps_handler) - 1);
  assert_int_equal(
      az_span_size(update_manifest.instructions.steps[0].handler),
      sizeof(instructions_steps_handler) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.instructions.steps[0].files[0]),
      instructions_steps_file,
      sizeof(instructions_steps_file) - 1);
  assert_int_equal(
      az_span_size(update_manifest.instructions.steps[0].files[0]),
      sizeof(instructions_steps_file) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.instructions.steps[0].handler_properties.installed_criteria),
      instructions_steps_handler_properties_install_criteria,
      sizeof(instructions_steps_handler_properties_install_criteria) - 1);
  assert_int_equal(
      az_span_size(update_manifest.instructions.steps[0].handler_properties.installed_criteria),
      sizeof(instructions_steps_handler_properties_install_criteria) - 1);
  assert_memory_equal(az_span_ptr(update_manifest.files[0].id), files_id, sizeof(files_id) - 1);
  assert_int_equal(az_span_size(update_manifest.files[0].id), sizeof(files_id) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.files[0].file_name), files_filename, sizeof(files_filename) - 1);
  assert_int_equal(az_span_size(update_manifest.files[0].file_name), sizeof(files_filename) - 1);
  assert_int_equal(update_manifest.files[0].size_in_bytes, files_size_in_bytes);
  assert_memory_equal(
      az_span_ptr(update_manifest.files[0].hashes[0].hash_type),
      files_hash_id,
      sizeof(files_hash_id) - 1);
  assert_int_equal(
      az_span_size(update_manifest.files[0].hashes[0].hash_type), sizeof(files_hash_id) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.files[0].hashes[0].hash_value),
      files_hashes_sha,
      sizeof(files_hashes_sha) - 1);
  assert_int_equal(
      az_span_size(update_manifest.files[0].hashes[0].hash_value), sizeof(files_hashes_sha) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.create_date_time),
      created_date_time,
      sizeof(created_date_time) - 1);
  assert_int_equal(az_span_size(update_manifest.create_date_time), sizeof(created_date_time) - 1);
}

static void test_az_iot_adu_client_parse_update_manifest_payload_reverse_order_succeed(void** state)
{
  (void)state;

  az_iot_adu_client adu_client;
  az_json_reader reader;
  az_iot_adu_client_update_manifest update_manifest;

  assert_int_equal(az_iot_adu_client_init(&adu_client, NULL), AZ_OK);

  assert_int_equal(
      az_json_reader_init(
          &reader,
          az_span_create(
              adu_request_manifest_reverse_order, sizeof(adu_request_manifest_reverse_order) - 1),
          NULL),
      AZ_OK);

  assert_int_equal(
      az_iot_adu_client_parse_update_manifest(&adu_client, &reader, &update_manifest), AZ_OK);

  assert_memory_equal(
      az_span_ptr(update_manifest.manifest_version),
      manifest_version,
      sizeof(manifest_version) - 1);
  assert_int_equal(az_span_size(update_manifest.manifest_version), sizeof(manifest_version) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.update_id.provider),
      update_id_provider,
      sizeof(update_id_provider) - 1);
  assert_int_equal(
      az_span_size(update_manifest.update_id.provider), sizeof(update_id_provider) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.update_id.name), update_id_name, sizeof(update_id_name) - 1);
  assert_int_equal(az_span_size(update_manifest.update_id.name), sizeof(update_id_name) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.update_id.version),
      update_id_version,
      sizeof(update_id_version) - 1);
  assert_int_equal(az_span_size(update_manifest.update_id.version), sizeof(update_id_version) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.instructions.steps[0].handler),
      instructions_steps_handler,
      sizeof(instructions_steps_handler) - 1);
  assert_int_equal(
      az_span_size(update_manifest.instructions.steps[0].handler),
      sizeof(instructions_steps_handler) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.instructions.steps[0].files[0]),
      instructions_steps_file,
      sizeof(instructions_steps_file) - 1);
  assert_int_equal(
      az_span_size(update_manifest.instructions.steps[0].files[0]),
      sizeof(instructions_steps_file) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.instructions.steps[0].handler_properties.installed_criteria),
      instructions_steps_handler_properties_install_criteria,
      sizeof(instructions_steps_handler_properties_install_criteria) - 1);
  assert_int_equal(
      az_span_size(update_manifest.instructions.steps[0].handler_properties.installed_criteria),
      sizeof(instructions_steps_handler_properties_install_criteria) - 1);
  assert_memory_equal(az_span_ptr(update_manifest.files[0].id), files_id, sizeof(files_id) - 1);
  assert_int_equal(az_span_size(update_manifest.files[0].id), sizeof(files_id) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.files[0].file_name), files_filename, sizeof(files_filename) - 1);
  assert_int_equal(az_span_size(update_manifest.files[0].file_name), sizeof(files_filename) - 1);
  assert_int_equal(update_manifest.files[0].size_in_bytes, files_size_in_bytes);
  assert_memory_equal(
      az_span_ptr(update_manifest.files[0].hashes[0].hash_type),
      files_hash_id,
      sizeof(files_hash_id) - 1);
  assert_int_equal(
      az_span_size(update_manifest.files[0].hashes[0].hash_type), sizeof(files_hash_id) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.files[0].hashes[0].hash_value),
      files_hashes_sha,
      sizeof(files_hashes_sha) - 1);
  assert_int_equal(
      az_span_size(update_manifest.files[0].hashes[0].hash_value), sizeof(files_hashes_sha) - 1);
  assert_memory_equal(
      az_span_ptr(update_manifest.create_date_time),
      created_date_time,
      sizeof(created_date_time) - 1);
  assert_int_equal(az_span_size(update_manifest.create_date_time), sizeof(created_date_time) - 1);
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_adu()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    // Precondition Tests
    cmocka_unit_test(test_az_iot_adu_client_init_NULL_client_fail),
    cmocka_unit_test(test_az_iot_adu_is_component_device_update_NULL_client_fail),
    cmocka_unit_test(test_az_iot_adu_client_get_agent_state_payload_NULL_client_fail),
    cmocka_unit_test(test_az_iot_adu_client_get_agent_state_payload_NULL_device_info_fail),
    cmocka_unit_test(test_az_iot_adu_client_get_service_properties_response_NULL_client_fail),
    cmocka_unit_test(test_az_iot_adu_client_get_service_properties_response_NULL_json_writer_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_service_properties_NULL_client_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_service_properties_NULL_reader_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_service_properties_empty_buffer_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_service_properties_NULL_request_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_update_manifest_NULL_client_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_update_manifest_NULL_reader_fail),
    cmocka_unit_test(test_az_iot_adu_client_parse_update_manifest_NULL_update_manifest_fail),
#endif // AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_adu_client_init_succeed),
    cmocka_unit_test(test_az_iot_adu_is_component_device_update_succeed),
    cmocka_unit_test(test_az_iot_adu_client_get_agent_state_payload_succeed),
    cmocka_unit_test(test_az_iot_adu_client_get_agent_state_long_payload_succeed),
    cmocka_unit_test(test_az_iot_adu_client_get_service_properties_response_succeed),
    cmocka_unit_test(test_az_iot_adu_client_parse_service_properties_succeed),
    cmocka_unit_test(test_az_iot_adu_client_parse_service_properties_payload_reverse_order_succeed),
    cmocka_unit_test(test_az_iot_adu_client_parse_update_manifest_succeed),
    cmocka_unit_test(test_az_iot_adu_client_parse_update_manifest_payload_reverse_order_succeed),
  };
  return cmocka_run_group_tests_name("az_iot_adu", tests, NULL, NULL);
}
