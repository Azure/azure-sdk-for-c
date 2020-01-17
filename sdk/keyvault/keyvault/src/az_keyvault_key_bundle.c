#include <az_json_data.h>
#include <az_json_parser.h>
#include <az_json_token.h>
#include <az_keyvault_key_bundle.h>
#include <az_optional_bool.h>
#include <az_pair.h>
#include <az_result.h>
#include <az_span.h>
#include <az_span_builder.h>
#include <az_span_span.h>
#include <az_str.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <_az_cfg.h>

AZ_NODISCARD az_result _az_span_builder_write_pair_span(
    az_span_builder * const builder,
    az_json_parser * const parser,
    az_json_token const token,
    az_pair_span * const out) {
  switch (token.kind) {
    case AZ_JSON_TOKEN_OBJECT: {
      size_t const buffer_size = builder->buffer.size;
      while (true) {
        az_json_token_member token_member = { 0 };
        az_result const result = az_json_parser_read_object_member(parser, &token_member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        az_pair data_member = { 0 };
        AZ_RETURN_IF_FAILED(
            az_span_builder_write_json_string(builder, token_member.name, &data_member.key));
        // check that only string is expected here. First json level

        if (token_member.value.kind != AZ_JSON_TOKEN_STRING) {
          return AZ_ERROR_JSON_INVALID_STATE;
        }

        AZ_RETURN_IF_FAILED(az_span_builder_write_json_string(
            builder, token_member.value.data.string, &data_member.value));

        AZ_RETURN_IF_FAILED(az_span_builder_top_aligned_append(builder, _AZ_DATA(data_member)));
      }
      void const * p = { 0 };
      size_t i = 0;
      AZ_RETURN_IF_FAILED(
          az_span_builder_top_array_revert(builder, _AZ_TYPE(az_pair), buffer_size, &p, &i));
      *out = (az_pair_span){ .begin = p, .size = i };
      break;
    }
    default: { return AZ_ERROR_JSON_INVALID_STATE; }
  }
  return AZ_OK;
}

AZ_NODISCARD az_result _az_span_builder_write_key_bundle(
    az_span_builder * const builder,
    az_json_parser * const parser,
    az_json_token const token,
    az_keyvault_key_bundle * const out) {
  switch (token.kind) {
    case AZ_JSON_TOKEN_OBJECT: {
      size_t const buffer_size = builder->buffer.size;
      while (true) {
        az_json_token_member token_member = { 0 };
        az_result const result = az_json_parser_read_object_member(parser, &token_member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);

        // TODO: Compare as json str and not as span
        if (az_span_is_equal(token_member.name, AZ_STR("tags"))) {
          out->tags.is_present = true;
          AZ_RETURN_IF_FAILED(_az_span_builder_write_pair_span(
              builder, parser, token_member.value, &out->tags.data));
        } else {
          return AZ_ERROR_JSON_INVALID_STATE;
        }
      }
      break;
    }
    default: { return AZ_ERROR_JSON_INVALID_STATE; }
  }
  return AZ_OK;
}

AZ_INLINE AZ_NODISCARD az_result _az_span_builder_append_key_bundle(
    az_span_builder * const builder,
    az_keyvault_key_bundle const data,
    az_keyvault_key_bundle const ** const out) {
  void const * p = { 0 };
  AZ_RETURN_IF_FAILED(az_span_builder_aligned_append(builder, _AZ_DATA(data), &p));
  *out = (az_keyvault_key_bundle const *)p;
  return AZ_OK;
}

AZ_NODISCARD az_result az_keyvault_json_to_key_bundle(
    az_span const json,
    az_mut_span const buffer,
    az_keyvault_key_bundle const ** const out) {

  az_json_parser parser = az_json_parser_create(json);
  az_span_builder builder = az_span_builder_create(buffer);
  az_json_token token = { 0 };

  AZ_RETURN_IF_FAILED(az_json_parser_read(&parser, &token));

  az_keyvault_key_bundle data = { 0 };
  AZ_RETURN_IF_FAILED(_az_span_builder_write_key_bundle(&builder, &parser, token, &data));
  return _az_span_builder_append_key_bundle(&builder, data, out);
}
