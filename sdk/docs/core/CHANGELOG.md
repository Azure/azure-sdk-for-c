# Release History


## 1.0.0-preview.4 (Unreleased)


## 1.0.0-preview.3 (2020-07-20)

- Updated az_result values:
  - Rename az_result value `AZ_ERROR_PARSER_UNEXPECTED_CHAR` to `AZ_ERROR_UNEXPECTED_CHAR`
  - Remove unused az_result error codes: `AZ_ERROR_JSON_STRING_END`, `AZ_ERROR_JSON_POINTER_TOKEN_END`, and `AZ_ERROR_MUTEX`
- Add permutations of numeric type parsing and formatting APIs on span - ato[u|i][32|64], atod and the inverse [u|i][32|64]toa, dtoa.
- Updates to the JSON APIs:
  - Rename JSON parser/builder APIs to reader/writer
  - Add double parsing and formatting support to JSON reader and JSON writer
  - Redesign JSON reader and JSON token APIs with lazy evaluation of tokens, proper unescaping support, and hardened validation
- Update samples, README docs along with deep dive video, and VSCode and CMake instructions
- Add log classification for IoT Convenience layer
- Fixes on SAS token generation - iImplemented URL-encoding of components of SAS tokens
- Rename the http response function `az_http_response_write_span` to `az_http_response_append`
- Add thread safety for Client Secret Credential
- Transform apply_credential into a HTTP policy

## 1.0.0-preview.2 (2020-05-18)
- Update top-level CMakeLists.txt to only add subdirectory for specified platform.
- Add compilation option to remove all logging from SDK code.

## 1.0.0-preview.1 (2020-05-12)
Initial release. Please see the [README](https://github.com/Azure/azure-sdk-for-c/blob/master/README.md) for more information.
