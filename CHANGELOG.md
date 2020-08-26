# Release History

## 1.0.0-preview.5 (Unreleased)

### New Features

- Add `az_json_writer_append_json_text()` to support appending existing JSON with the JSON writer.
- Add support for system properties for IoT Hub messages to `az_iot_common.h`.

### Breaking Changes

- Rename `az_iot_hub_client_properties` to `az_iot_message_properties` and move it from `az_iot_hub_client.h` to `az_iot_common.h`.

### Bug Fixes

### Other Changes and Improvements

## 1.0.0-preview.4 (2020-08-10)

### New Features

- Support for writing JSON to non-contiguous buffers.
- Support for reading JSON from non-contiguous buffers.
- Add support for national cloud auth URLs.

### Breaking Changes

- `az_span.h`:
  - `az_span_init()` is renamed to `az_span_create()`.
  - `az_span_from_str()` is renamed to `az_span_create_from_str()`.
  - Removed `az_pair_from_str()`.
- `az_context`:
  - `key` and `value` are `const`.
  - `az_context_with_expiration()` is renamed to `az_context_create_with_expiration()`.
  - `az_context_with_value()` is renamed to `az_context_create_with_value()`.
  - `az_context_app` is renamed to `az_context_application`.
- `az_credential_client_secret_init()` now takes fourth parameter, `authority`.
- `az_http_policy_retry_options`:
  - `status_codes` now should be terminated by `AZ_HTTP_STATUS_CODE_END_OF_LIST`.
  - `max_retries` is now `int32_t` instead of `int16_t`.
- `az_config.h`:
  - `AZ_HTTP_REQUEST_URL_BUF_SIZE` renamed to `AZ_HTTP_REQUEST_URL_BUFFER_SIZE`.
  - `AZ_HTTP_REQUEST_BODY_BUF_SIZE` renamed to `AZ_HTTP_REQUEST_BODY_BUFFER_SIZE`.
  - `AZ_LOG_MSG_BUF_SIZE` renamed to `AZ_LOG_MESSAGE_BUFFER_SIZE`.
- `az_result`:
  - `AZ_ERROR_HTTP_PLATFORM` renamed to `AZ_ERROR_HTTP_ADAPTER`.
  - `AZ_ERROR_EOF` renamed to `AZ_ERROR_UNEXPECTED_END`.
  - Removed `AZ_CONTINUE`.
- `az_storage_blobs_blob_client`:
  - `retry` field renamed to `retry_options` in `az_storage_blobs_blob_client_options`.
  - Moved `az_context* context` parameter from `az_storage_blobs_blob_upload()` into a public field on `az_storage_blobs_blob_upload_options`.
- `az_json_writer`:
  - `az_json_writer_get_json()` is renamed to `az_json_writer_get_bytes_used_in_destination()`.

### Bug Fixes

- Remove support for non-finite double values while parsing/formatting.
- Use custom, portable implementation of IEEE 754 compliant `isfinite()` since some embedded platforms don't have it.
- Limit use of `sscanf` only to double parsing, using a custom implementation for {u}int{32|64} parsing because of incompatibility with `sscanf` format and the `GCC newlib-nano` implementation.

### Other Changes and Improvements

- Made `az_http_request` and related APIs to get URL, body, and headers, public.
- Add and update IoT samples, including DPS.
- Add samples for IoT Hub Plug and Play.

## 1.0.0-preview.3 (2020-07-20)

- Updated `az_result` values:
  - Rename `az_result` value `AZ_ERROR_PARSER_UNEXPECTED_CHAR` to `AZ_ERROR_UNEXPECTED_CHAR`.
  - Remove unused `az_result` error codes: `AZ_ERROR_JSON_STRING_END`, `AZ_ERROR_JSON_POINTER_TOKEN_END`, and `AZ_ERROR_MUTEX`.
- Add permutations of numeric type parsing and formatting APIs on span - ato[u|i][32|64], atod and the inverse [u|i][32|64]toa, dtoa.
- Updates to the JSON APIs:
  - Rename JSON parser/builder APIs to reader/writer.
  - Add double parsing and formatting support to JSON reader and JSON writer.
  - Redesign JSON reader and JSON token APIs with lazy evaluation of tokens, proper unescaping support, and hardened validation.
- Update samples, README docs along with deep dive video, and VSCode and CMake instructions.
  - Add PnP sample for Azure IoT Hub.
- Add log classification for the IoT convenience layer.
- Fixed SAS token generation by URL-encoding the components.
- Rename the http response function `az_http_response_write_span` to `az_http_response_append`.
- Add thread safety for client secret credential.
- Transform `apply_credential` into an HTTP policy.
- Default behavior for failed preconditions changed to infinite loop instead of thread sleep.

## 1.0.0-preview.2 (2020-05-18)

- Update top-level CMakeLists.txt to only add subdirectory for specified platform.
- Add compilation option to remove all logging from SDK code.

## 1.0.0-preview.1 (2020-05-12)

Initial release. Please see the [README](https://github.com/Azure/azure-sdk-for-c/blob/master/README.md) for more information.
