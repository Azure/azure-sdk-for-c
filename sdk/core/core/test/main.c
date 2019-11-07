// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_base64.h>
#include <az_http_request.h>
#include <az_http_request_builder.h>
#include <az_json_parser.h>
#include <az_span_builder.h>
#include <az_span_emitter.h>
#include <az_span_reader.h>
#include <az_uri.h>

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./az_test.h"
#include "./test_http_response_parser.h"
#include "./test_json_value.h"
#include "./test_json_pointer_parser.h"

#include <_az_cfg.h>

int exit_code = 0;

az_result write(az_mut_span const output, size_t * const o, az_span const s) {
  for (size_t i = 0; i != s.size; ++i, ++*o) {
    if (*o == output.size) {
      return 1;
    }
    output.begin[*o] = s.begin[i];
  }
  return 0;
}

az_result write_str(az_mut_span const output, size_t * o, az_span const s) {
  AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("\"")));
  AZ_RETURN_IF_FAILED(write(output, o, s));
  AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("\"")));
  return AZ_OK;
}

az_result read_write_value(
    az_mut_span const output,
    size_t * o,
    az_json_parser * const state,
    az_json_value const value) {
  switch (value.kind) {
    case AZ_JSON_VALUE_NULL:
      return write(output, o, AZ_STR("null"));
    case AZ_JSON_VALUE_BOOLEAN:
      return write(output, o, value.data.boolean ? AZ_STR("true") : AZ_STR("false"));
    case AZ_JSON_VALUE_NUMBER:
      return write(output, o, AZ_STR("0"));
    case AZ_JSON_VALUE_STRING:
      return write_str(output, o, value.data.string);
    case AZ_JSON_VALUE_OBJECT: {
      AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("{")));
      bool need_comma = false;
      while (true) {
        az_json_member member;
        az_result const result = az_json_parser_get_object_member(state, &member);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        if (need_comma) {
          AZ_RETURN_IF_FAILED(write(output, o, AZ_STR(",")));
        } else {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(write_str(output, o, member.name));
        AZ_RETURN_IF_FAILED(write(output, o, AZ_STR(":")));
        AZ_RETURN_IF_FAILED(read_write_value(output, o, state, member.value));
      }
      return write(output, o, AZ_STR("}"));
    }
    case AZ_JSON_VALUE_ARRAY: {
      AZ_RETURN_IF_FAILED(write(output, o, AZ_STR("[")));
      bool need_comma = false;
      while (true) {
        az_json_value element;
        az_result const result = az_json_parser_get_array_element(state, &element);
        if (result == AZ_ERROR_ITEM_NOT_FOUND) {
          break;
        }
        AZ_RETURN_IF_FAILED(result);
        if (need_comma) {
          AZ_RETURN_IF_FAILED(write(output, o, AZ_STR(",")));
        } else {
          need_comma = true;
        }
        AZ_RETURN_IF_FAILED(read_write_value(output, o, state, element));
      }
      return write(output, o, AZ_STR("]"));
    }
    default:
      break;
  }
  return AZ_ERROR_JSON_INVALID_STATE;
}

az_result read_write(az_span const input, az_mut_span const output, size_t * const o) {
  az_json_parser parser = az_json_parser_create(input);
  az_json_value value;
  AZ_RETURN_IF_FAILED(az_json_parser_get(&parser, &value));
  AZ_RETURN_IF_FAILED(read_write_value(output, o, &parser, value));
  return az_json_parser_done(&parser);
}

static az_span const sample1 = AZ_CONST_STR( //
    "{\n"
    "  \"parameters\": {\n"
    "    \"subscriptionId\": \"{subscription-id}\",\n"
    "      \"resourceGroupName\" : \"res4303\",\n"
    "      \"accountName\" : \"sto7280\",\n"
    "      \"containerName\" : \"container8723\",\n"
    "      \"api-version\" : \"2019-04-01\",\n"
    "      \"monitor\" : \"true\",\n"
    "      \"LegalHold\" : {\n"
    "      \"tags\": [\n"
    "        \"tag1\",\n"
    "          \"tag2\",\n"
    "          \"tag3\"\n"
    "      ]\n"
    "    }\n"
    "  },\n"
    "    \"responses\": {\n"
    "    \"200\": {\n"
    "      \"body\": {\n"
    "        \"hasLegalHold\": false,\n"
    "          \"tags\" : []\n"
    "      }\n"
    "    }\n"
    "  }\n"
    "}\n");

static az_span const b64_decoded0 = AZ_CONST_STR("");
static az_span const b64_decoded1 = AZ_CONST_STR("1");
static az_span const b64_decoded2 = AZ_CONST_STR("12");
static az_span const b64_decoded3 = AZ_CONST_STR("123");
static az_span const b64_decoded4 = AZ_CONST_STR("1234");
static az_span const b64_decoded5 = AZ_CONST_STR("12345");
static az_span const b64_decoded6 = AZ_CONST_STR("123456");

static az_span const b64_encoded0 = AZ_CONST_STR("");
static az_span const b64_encoded1 = AZ_CONST_STR("MQ==");
static az_span const b64_encoded2 = AZ_CONST_STR("MTI=");
static az_span const b64_encoded3 = AZ_CONST_STR("MTIz");
static az_span const b64_encoded4 = AZ_CONST_STR("MTIzNA==");
static az_span const b64_encoded5 = AZ_CONST_STR("MTIzNDU=");
static az_span const b64_encoded6 = AZ_CONST_STR("MTIzNDU2");

static az_span const b64_encoded0u = AZ_CONST_STR("");
static az_span const b64_encoded1u = AZ_CONST_STR("MQ");
static az_span const b64_encoded2u = AZ_CONST_STR("MTI");
static az_span const b64_encoded3u = AZ_CONST_STR("MTIz");
static az_span const b64_encoded4u = AZ_CONST_STR("MTIzNA");
static az_span const b64_encoded5u = AZ_CONST_STR("MTIzNDU");
static az_span const b64_encoded6u = AZ_CONST_STR("MTIzNDU2");

static az_span const b64_encoded_bin1
    = AZ_CONST_STR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

static az_span const b64_encoded_bin1u
    = AZ_CONST_STR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");

static uint8_t const b64_decoded_bin1_buf[]
    = { 0x00, 0x10, 0x83, 0x10, 0x51, 0x87, 0x20, 0x92, 0x8B, 0x30, 0xD3, 0x8F,
        0x41, 0x14, 0x93, 0x51, 0x55, 0x97, 0x61, 0x96, 0x9B, 0x71, 0xD7, 0x9F,
        0x82, 0x18, 0xA3, 0x92, 0x59, 0xA7, 0xA2, 0x9A, 0xAB, 0xB2, 0xDB, 0xAF,
        0xC3, 0x1C, 0xB3, 0xD3, 0x5D, 0xB7, 0xE3, 0x9E, 0xBB, 0xF3, 0xDF, 0xBF };

static az_span const b64_decoded_bin1
    = { .begin = b64_decoded_bin1_buf, .size = sizeof(b64_decoded_bin1_buf) };

static az_span const b64_encoded_bin2
    = AZ_CONST_STR("/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+zQ==");

static az_span const b64_encoded_bin2u
    = AZ_CONST_STR("_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-zQ");

static uint8_t const b64_decoded_bin2_buf[]
    = { 0xFC, 0x00, 0x42, 0x0C, 0x41, 0x46, 0x1C, 0x82, 0x4A, 0x2C, 0xC3, 0x4E, 0x3D,
        0x04, 0x52, 0x4D, 0x45, 0x56, 0x5D, 0x86, 0x5A, 0x6D, 0xC7, 0x5E, 0x7E, 0x08,
        0x62, 0x8E, 0x49, 0x66, 0x9E, 0x8A, 0x6A, 0xAE, 0xCB, 0x6E, 0xBF, 0x0C, 0x72,
        0xCF, 0x4D, 0x76, 0xDF, 0x8E, 0x7A, 0xEF, 0xCF, 0x7E, 0xCD };

static az_span const b64_decoded_bin2
    = { .begin = b64_decoded_bin2_buf, .size = sizeof(b64_decoded_bin2_buf) };

static az_span const b64_encoded_bin3
    = AZ_CONST_STR("V/ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+zQ=");

static az_span const b64_encoded_bin3u
    = AZ_CONST_STR("V_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-zQ");

static uint8_t const b64_decoded_bin3_buf[]
    = { 0x57, 0xF0, 0x01, 0x08, 0x31, 0x05, 0x18, 0x72, 0x09, 0x28, 0xB3, 0x0D, 0x38,
        0xF4, 0x11, 0x49, 0x35, 0x15, 0x59, 0x76, 0x19, 0x69, 0xB7, 0x1D, 0x79, 0xF8,
        0x21, 0x8A, 0x39, 0x25, 0x9A, 0x7A, 0x29, 0xAA, 0xBB, 0x2D, 0xBA, 0xFC, 0x31,
        0xCB, 0x3D, 0x35, 0xDB, 0x7E, 0x39, 0xEB, 0xBF, 0x3D, 0xFB, 0x34 };

static az_span const b64_decoded_bin3
    = { .begin = b64_decoded_bin3_buf, .size = sizeof(b64_decoded_bin3_buf) };

static az_span const uri_encoded = AZ_CONST_STR(
    "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%"
    "1F%20%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F0123456789%3A%3B%3C%3D%3E%3F%"
    "40ABCDEFGHIJKLMNOPQRSTUVWXYZ%5B%5C%5D%5E_%60abcdefghijklmnopqrstuvwxyz%7B%7C%7D~%7F%80%81%82%"
    "83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F%A0%A1%"
    "A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF%C0%"
    "C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF%"
    "E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%"
    "FF");

static az_span const uri_encoded2 = AZ_CONST_STR(
    "%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%"
    "1F%20%21%22%23%24%25%26%27%28%29%2A%2B%2C%2D%2E%2F%30%31%32%33%34%35%36%37%38%39%3A%3B%3C%3D%"
    "3E%3F%40%41%42%43%44%45%46%47%48%49%4A%4B%4C%4D%4E%4F%50%51%52%53%54%55%56%57%58%59%5A%5B%5C%"
    "5D%5E%5F%60%61%62%63%64%65%66%67%68%69%6A%6B%6C%6D%6E%6F%70%71%72%73%74%75%76%77%78%79%7A%7B%"
    "7C%7D%7E%7F%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F%90%91%92%93%94%95%96%97%98%99%9A%"
    "9B%9C%9D%9E%9F%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%"
    "BA%BB%BC%BD%BE%BF%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF%D0%D1%D2%D3%D4%D5%D6%D7%D8%"
    "D9%DA%DB%DC%DD%DE%DF%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF%F0%F1%F2%F3%F4%F5%F6%F7%"
    "F8%F9%FA%FB%FC%FD%FE%FF");

static az_span const uri_encoded3 = AZ_CONST_STR(
    "%00%01%02%03%04%05%06%07%08%09%0a%0b%0c%0d%0e%0f%10%11%12%13%14%15%16%17%18%19%1a%1b%1c%1d%1e%"
    "1f%20%21%22%23%24%25%26%27%28%29%2a%2b%2c%2d%2e%2f%30%31%32%33%34%35%36%37%38%39%3a%3b%3c%3d%"
    "3e%3f%40%41%42%43%44%45%46%47%48%49%4a%4b%4c%4d%4e%4f%50%51%52%53%54%55%56%57%58%59%5a%5b%5c%"
    "5d%5e%5f%60%61%62%63%64%65%66%67%68%69%6a%6b%6c%6d%6e%6f%70%71%72%73%74%75%76%77%78%79%7a%7b%"
    "7c%7d%7e%7f%80%81%82%83%84%85%86%87%88%89%8a%8b%8c%8d%8e%8f%90%91%92%93%94%95%96%97%98%99%9a%"
    "9b%9c%9d%9e%9f%a0%a1%a2%a3%a4%a5%a6%a7%a8%a9%aa%ab%ac%ad%ae%af%b0%b1%b2%b3%b4%b5%b6%b7%b8%b9%"
    "ba%bb%bc%bd%be%bf%c0%c1%c2%c3%c4%c5%c6%c7%c8%c9%ca%cb%cc%cd%ce%cf%d0%d1%d2%d3%d4%d5%d6%d7%d8%"
    "d9%da%db%dc%dd%de%df%e0%e1%e2%e3%e4%e5%e6%e7%e8%e9%ea%eb%ec%ed%ee%ef%f0%f1%f2%f3%f4%f5%f6%f7%"
    "f8%f9%fa%fb%fc%fd%fe%ff");

static uint8_t const uri_decoded_buf[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

static az_span const uri_decoded = { .begin = uri_decoded_buf, .size = sizeof(uri_decoded_buf) };

static az_span hrb_url = AZ_CONST_STR("https://antk-keyvault.vault.azure.net/secrets/Password");

static az_span hrb_param_api_version_name = AZ_CONST_STR("api-version");
static az_span hrb_param_api_version_value = AZ_CONST_STR("7.0");

static az_span hrb_url2
    = AZ_CONST_STR("https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0");

static az_span hrb_param_test_param_name = AZ_CONST_STR("test-param");
static az_span hrb_param_test_param_value = AZ_CONST_STR("value");

static az_span hrb_url3 = AZ_CONST_STR(
    "https://antk-keyvault.vault.azure.net/secrets/Password?api-version=7.0&test-param=value");

static az_span hrb_header_content_type_name = AZ_CONST_STR("Content-Type");
static az_span hrb_header_content_type_value = AZ_CONST_STR("application/x-www-form-urlencoded");

static az_span hrb_header_authorization_name = AZ_CONST_STR("authorization");
static az_span hrb_header_authorization_value1 = AZ_CONST_STR("Bearer 123456789");
static az_span hrb_header_authorization_value2 = AZ_CONST_STR("Bearer 99887766554433221100");

int main() {
  {
    az_json_parser state = az_json_parser_create(AZ_STR("    "));
    TEST_ASSERT(az_json_parser_get(&state, NULL) == AZ_ERROR_ARG);
  }
  {
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(NULL, &value) == AZ_ERROR_ARG);
  }
  {
    az_json_parser parser = az_json_parser_create(AZ_STR("    "));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&parser, &value) == AZ_ERROR_EOF);
  }
  {
    az_json_parser parser = az_json_parser_create(AZ_STR("  null  "));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&parser, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NULL);
    TEST_ASSERT(az_json_parser_done(&parser) == AZ_OK);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR("  nul"));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_ERROR_EOF);
  }
  {
    az_json_parser parser = az_json_parser_create(AZ_STR("  false"));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&parser, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == false);
    TEST_ASSERT(az_json_parser_done(&parser) == AZ_OK);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR("  falsx  "));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR("true "));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == true);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR("  truem"));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_span const s = AZ_STR(" \"tr\\\"ue\\t\" ");
    az_json_parser state = az_json_parser_create(s);
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(value.data.string.begin == s.begin + 2);
    TEST_ASSERT(value.data.string.size == 8);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_span const s = AZ_STR("\"\\uFf0F\"");
    az_json_parser state = az_json_parser_create(s);
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(value.data.string.begin == s.begin + 1);
    TEST_ASSERT(value.data.string.size == 6);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_span const s = AZ_STR("\"\\uFf0\"");
    az_json_parser state = az_json_parser_create(s);
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_ERROR_PARSER_UNEXPECTED_CHAR);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR(" 23 "));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == 23);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR(" -23.56"));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == -23.56);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR(" -23.56e-3"));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    TEST_ASSERT(value.data.number == -0.02356);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_json_parser state = az_json_parser_create(AZ_STR(" [ true, 0.3 ]"));
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_ARRAY);
    TEST_ASSERT(az_json_parser_get_array_element(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_BOOLEAN);
    TEST_ASSERT(value.data.boolean == true);
    TEST_ASSERT(az_json_parser_get_array_element(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_NUMBER);
    // TEST_ASSERT(value.val.number == 0.3);
    TEST_ASSERT(az_json_parser_get_array_element(&state, &value) == AZ_ERROR_ITEM_NOT_FOUND);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  {
    az_span const json = AZ_STR("{\"a\":\"Hello world!\"}");
    az_json_parser state = az_json_parser_create(json);
    az_json_value value;
    TEST_ASSERT(az_json_parser_get(&state, &value) == AZ_OK);
    TEST_ASSERT(value.kind == AZ_JSON_VALUE_OBJECT);
    az_json_member member;
    TEST_ASSERT(az_json_parser_get_object_member(&state, &member) == AZ_OK);
    TEST_ASSERT(member.name.begin == json.begin + 2);
    TEST_ASSERT(member.name.size == 1);
    TEST_ASSERT(member.value.kind == AZ_JSON_VALUE_STRING);
    TEST_ASSERT(member.value.data.string.begin == json.begin + 6);
    TEST_ASSERT(member.value.data.string.size == 12);
    TEST_ASSERT(az_json_parser_get_object_member(&state, &member) == AZ_ERROR_ITEM_NOT_FOUND);
    TEST_ASSERT(az_json_parser_done(&state) == AZ_OK);
  }
  uint8_t buffer[1000];
  az_mut_span const output = { .begin = buffer, .size = 1000 };
  {
    size_t o = 0;
    TEST_ASSERT(
        read_write(AZ_STR("{ \"a\" : [ true, { \"b\": [{}]}, 15 ] }"), output, &o) == AZ_OK);
    az_span const x = az_span_sub(az_mut_span_to_span(output), 0, o);
    TEST_ASSERT(az_span_eq(x, AZ_STR("{\"a\":[true,{\"b\":[{}]},0]}")));
  }
  {
    size_t o = 0;
    az_span const json = AZ_STR(
        // 0           1           2           3           4           5 6
        // 01234 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234
        // 56789 0123
        "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
        "[[[[[ [[[[");
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_ERROR_JSON_STACK_OVERFLOW);
  }
  {
    size_t o = 0;
    az_span const json = AZ_STR(
        // 0           1           2           3           4           5 6 01234
        // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
        "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
        "[[[[[ [[[");
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_ERROR_EOF);
  }
  {
    size_t o = 0;
    az_span const json = AZ_STR(
        // 0           1           2           3           4           5 6 01234
        // 56789 01234 56678 01234 56789 01234 56789 01234 56789 01234 56789 012
        "[[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ [[[[[ "
        "[[[[[ [[{"
        "   \"\\t\\n\": \"\\u0abc\"   "
        "}]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] ]]]]] "
        "]]]]] ]]]");
    az_result const result = read_write(json, output, &o);
    TEST_ASSERT(result == AZ_OK);
    az_span const x = az_span_sub(az_mut_span_to_span(output), 0, o);
    TEST_ASSERT(az_span_eq(
        x,
        AZ_STR( //
            "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[{"
            "\"\\t\\n\":\"\\u0abc\""
            "}]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]"
            "]")));
  }
  //
  {
    size_t o = 0;
    az_result const result = read_write(sample1, output, &o);
    TEST_ASSERT(result == AZ_OK);
  }

  // HTTP Builder
  {
    az_pair const query_array[] = {
      { .key = AZ_STR("hello"), .value = AZ_STR("world!") },
      { .key = AZ_STR("x"), .value = AZ_STR("42") },
    };
    az_pair_span const query = AZ_SPAN_FROM_ARRAY(query_array);
    //
    az_pair const headers_array[] = {
      { .key = AZ_STR("some"), .value = AZ_STR("xml") },
      { .key = AZ_STR("xyz"), .value = AZ_STR("very_long") },
    };
    az_pair_span const headers = AZ_SPAN_FROM_ARRAY(headers_array);
    //
    az_http_request const request = {
      .method = AZ_STR("GET"),
      .path = AZ_STR("/foo"),
      .query = az_pair_span_emit_action(&query),
      .headers = az_pair_span_emit_action(&headers),
      .body = AZ_STR("{ \"somejson\": true }"),
    };
    uint8_t buffer[1024];
    {
      az_span_builder wi = az_span_builder_create((az_mut_span)AZ_SPAN_FROM_ARRAY(buffer));
      az_span_action sv = az_span_builder_append_action(&wi);
      az_span const expected = AZ_STR( //
          "GET /foo?hello=world!&x=42 HTTP/1.1\r\n"
          "some: xml\r\n"
          "xyz: very_long\r\n"
          "\r\n"
          "{ \"somejson\": true }");
      az_result const result = az_http_request_emit_span_seq(&request, sv);
      TEST_ASSERT(result == AZ_OK);
      az_span const out = az_span_builder_result(&wi);
      TEST_ASSERT(az_span_eq(out, expected));
    }
    /*
    {
      printf("----Test: az_http_request_to_url_span\n");
      az_span_builder wi = az_span_builder_create((az_mut_span)AZ_SPAN(buffer));
      az_span_action sv = az_span_builder_append_action(&wi);
      az_const_span const expected = AZ_STR("/foo?hello=world!&x=42");
      az_result const result = az_build_url(&request, sv);
      TEST_ASSERT(result == AZ_OK);
      az_mut_span out = az_span_builder_result(&wi);
      TEST_ASSERT(az_const_span_eq(az_mut_span_to_const_span(out), expected));
    }
    // url size
    {
      printf("----Test: az_http_get_url_size\n");
      size_t x = 0;
      size_t const expected = 22;
      az_result const result = az_http_get_url_size(&request, &x);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(expected == x);
    }
    // url to str
    {
      printf("----Test: az_http_url_to_new_str\n");
      char * p;
      az_result const result = az_http_url_to_new_str(&request, &p);
      TEST_ASSERT(result == AZ_OK);
      TEST_ASSERT(strcmp(p, "/foo?hello=world!&x=42") == 0);
      free(p);
    }
    */
  }

  // span emitter size
  {
    az_span const array[] = {
      AZ_STR("Hello"),
      AZ_STR(" "),
      AZ_STR("world!"),
    };
    az_span_span const span = AZ_SPAN_FROM_ARRAY(array);
    az_span_emitter const emitter = az_span_span_emit_action(&span);
    size_t s = 42;
    az_result const result = az_span_emitter_size(emitter, &s);
    TEST_ASSERT(result == AZ_OK);
    TEST_ASSERT(s == 12);
  }

  {
    az_span const expected = AZ_STR("@###copy#copy#make some zero-terminated strings#make "
                                    "some\0zero-terminated\0strings\0####@");

    uint8_t buf[87];
    assert(expected.size == sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); ++i) {
      buf[i] = '@';
    }

    az_mut_span actual = { .begin = buf, .size = sizeof(buf) };
    az_mut_span_set((az_mut_span){ .begin = actual.begin + 1, .size = actual.size - 2 }, '#');

    az_mut_span result;

    char const phrase1[] = "copy";
    memcpy(actual.begin + 4, phrase1, sizeof(phrase1) - 1);
    az_mut_span_copy(
        (az_mut_span){ .begin = actual.begin + 9, .size = 4 },
        (az_span){ .begin = actual.begin + 4, .size = 4 },
        &result);

    char const phrase2[] = "make some zero-terminated strings";
    memcpy(actual.begin + 14, phrase2, sizeof(phrase2) - 1);

    az_span const make_some = (az_span){ .begin = actual.begin + 14, .size = 9 };
    az_span const zero_terminated = (az_span){ .begin = actual.begin + 24, .size = 15 };
    az_span const strings = (az_span){ .begin = actual.begin + 40, .size = 7 };

    az_mut_span_to_str((az_mut_span){ .begin = actual.begin + 48, .size = 10 }, make_some, &result);
    az_mut_span_to_str(
        (az_mut_span){ .begin = actual.begin + 58, .size = 16 }, zero_terminated, &result);
    az_mut_span_to_str((az_mut_span){ .begin = actual.begin + 74, .size = 8 }, strings, &result);

    result.begin[result.size - 1] = '$';
    az_mut_span_to_str(result, strings, &result);

    TEST_ASSERT(az_span_eq(az_mut_span_to_span(actual), expected));
  }
  {
    uint8_t buf[68];
    az_mut_span const buffer = { .begin = buf, .size = sizeof(buf) };
    az_span result;

    az_span const * const decoded_input[]
        = { &b64_decoded0, &b64_decoded1, &b64_decoded2,     &b64_decoded3,     &b64_decoded4,
            &b64_decoded5, &b64_decoded6, &b64_decoded_bin1, &b64_decoded_bin2, &b64_decoded_bin3 };

    az_span const * const encoded_input[]
        = { &b64_encoded0, &b64_encoded1, &b64_encoded2,     &b64_encoded3,     &b64_encoded4,
            &b64_encoded5, &b64_encoded6, &b64_encoded_bin1, &b64_encoded_bin2, &b64_encoded_bin3 };

    az_span const * const url_encoded_input[]
        = { &b64_encoded0u,     &b64_encoded1u,    &b64_encoded2u, &b64_encoded3u,
            &b64_encoded4u,     &b64_encoded5u,    &b64_encoded6u, &b64_encoded_bin1u,
            &b64_encoded_bin2u, &b64_encoded_bin3u };

    for (size_t i = 0; i < 10; ++i) {
      AZ_EXPECT_SUCCESS(az_base64_encode(false, buffer, *decoded_input[i], &result));
      TEST_ASSERT(az_span_eq(result, *encoded_input[i]));

      AZ_EXPECT_SUCCESS(az_base64_decode(buffer, *encoded_input[i], &result));
      TEST_ASSERT(az_span_eq(result, *decoded_input[i]));

      AZ_EXPECT_SUCCESS(az_base64_encode(true, buffer, *decoded_input[i], &result));
      TEST_ASSERT(az_span_eq(result, *url_encoded_input[i]));

      AZ_EXPECT_SUCCESS(az_base64_decode(buffer, *url_encoded_input[i], &result));
      TEST_ASSERT(az_span_eq(result, *decoded_input[i]));
    }
  }
  {
    uint8_t buf[256 * 3];
    az_mut_span const buffer = { .begin = buf, .size = sizeof(buf) };
    az_mut_span result;

    AZ_EXPECT_SUCCESS(az_uri_encode(buffer, AZ_STR("https://vault.azure.net"), &result));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(result), AZ_STR("https%3A%2F%2Fvault.azure.net")));

    AZ_EXPECT_SUCCESS(az_uri_decode(buffer, AZ_STR("https%3A%2F%2Fvault.azure.net"), &result));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(result), AZ_STR("https://vault.azure.net")));

    AZ_EXPECT_SUCCESS(az_uri_encode(buffer, uri_decoded, &result));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(result), uri_encoded));

    AZ_EXPECT_SUCCESS(az_uri_decode(buffer, uri_encoded, &result));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(result), uri_decoded));

    AZ_EXPECT_SUCCESS(az_uri_decode(buffer, uri_encoded2, &result));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(result), uri_decoded));

    AZ_EXPECT_SUCCESS(az_uri_decode(buffer, uri_encoded3, &result));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(result), uri_decoded));
  }
  {
    int16_t const url_max = 100;
    uint8_t buf[100 + (100 % 8) + (2 * sizeof(az_pair))];
    memset(buf, 0, sizeof(buf));
    az_mut_span const http_buf = { .begin = buf, .size = sizeof(buf) };
    az_http_request_builder hrb;

    AZ_EXPECT_SUCCESS(az_http_request_builder_init(&hrb, http_buf, 100, AZ_HTTP_METHOD_VERB_GET, hrb_url));
    TEST_ASSERT(az_span_eq(hrb.method_verb, AZ_HTTP_METHOD_VERB_GET));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(hrb.url), hrb_url));
    TEST_ASSERT(hrb.max_url_size == 100);
    TEST_ASSERT(hrb.max_headers == 2);
    TEST_ASSERT(hrb.headers_end == 0);
    TEST_ASSERT(hrb.retry_headers_start == 2);

    AZ_EXPECT_SUCCESS(az_http_request_builder_set_query_parameter(
        &hrb, hrb_param_api_version_name, hrb_param_api_version_value));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(hrb.url), hrb_url2));

    AZ_EXPECT_SUCCESS(az_http_request_builder_set_query_parameter(
        &hrb, hrb_param_test_param_name, hrb_param_test_param_value));
    TEST_ASSERT(az_span_eq(az_mut_span_to_span(hrb.url), hrb_url3));

    AZ_EXPECT_SUCCESS(az_http_request_builder_append_header(
        &hrb, hrb_header_content_type_name, hrb_header_content_type_value));

    TEST_ASSERT(hrb.headers_end == 1);
    TEST_ASSERT(hrb.retry_headers_start == 2);

    AZ_EXPECT_SUCCESS(az_http_request_builder_mark_retry_headers_start(&hrb));
    TEST_ASSERT(hrb.retry_headers_start == 1);

    AZ_EXPECT_SUCCESS(az_http_request_builder_append_header(
        &hrb, hrb_header_authorization_name, hrb_header_authorization_value1));
    TEST_ASSERT(hrb.headers_end == 2);
    TEST_ASSERT(hrb.retry_headers_start == 1);

    az_pair expected_headers1[2] = {
      { .key = hrb_header_content_type_name, .value = hrb_header_content_type_value },
      { .key = hrb_header_authorization_name, .value = hrb_header_authorization_value1 },
    };
    for (uint16_t i = 0; i < hrb.headers_end; ++i) {
      az_pair header = { 0 };
      AZ_EXPECT_SUCCESS(az_http_request_builder_get_header(&hrb, i, &header));

      TEST_ASSERT(az_span_eq(header.key, expected_headers1[i].key));
      TEST_ASSERT(az_span_eq(header.value, expected_headers1[i].value));
    }

    AZ_EXPECT_SUCCESS(az_http_request_builder_remove_retry_headers(&hrb));
    TEST_ASSERT(hrb.headers_end == 1);
    TEST_ASSERT(hrb.retry_headers_start == 1);

    AZ_EXPECT_SUCCESS(az_http_request_builder_append_header(
        &hrb, hrb_header_authorization_name, hrb_header_authorization_value2));
    TEST_ASSERT(hrb.headers_end == 2);
    TEST_ASSERT(hrb.retry_headers_start == 1);

    az_pair expected_headers2[2] = {
      { .key = hrb_header_content_type_name, .value = hrb_header_content_type_value },
      { .key = hrb_header_authorization_name, .value = hrb_header_authorization_value2 },
    };
    for (uint16_t i = 0; i < hrb.headers_end; ++i) {
      az_pair header = { 0 };
      AZ_EXPECT_SUCCESS(az_http_request_builder_get_header(&hrb, i, &header));
      TEST_ASSERT(az_span_eq(header.key, expected_headers2[i].key));
      TEST_ASSERT(az_span_eq(header.value, expected_headers2[i].value));
    }
  }

  test_http_response_parser();
  test_json_value();
  test_json_pointer_parser();
  return exit_code;
}
