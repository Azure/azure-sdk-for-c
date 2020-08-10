# Release History

## 1.0.0-preview.4 (Unreleased)

- API breaking changes:
  - `az_span.h`:
    - `az_span_init()` is renamed to `az_span_create()`.
    - `az_span_from_str()` is renamed to `az_span_create_from_str()`.
    - `az_allocator_context` is renamed to `az_span_allocator_context`.
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
