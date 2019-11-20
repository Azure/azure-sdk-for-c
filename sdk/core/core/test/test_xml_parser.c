// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include "./az_test.h"

#include <_az_cfg.h>
#include <az_str.h>
#include <az_xml_parser.h>

az_span get_test_sample_xml(){
  return AZ_STR(
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<Error>"
      "<Code>InvalidQueryParameterValue</Code>"
      "<Reason>invalid receipt format</Reason>"
      "<TestEmptyConentTag />"
    "</Error>");
}

void test_xml_parser() {
  az_xml_parser parser = az_xml_parser_create(get_test_sample_xml());

  az_xml_header header;
  az_span tag;
  az_xml_value value;
  TEST_ASSERT(az_xml_parser_read_header(&parser, &header) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("1.0"), header.version));
  TEST_ASSERT(az_span_eq(AZ_STR("utf-8"), header.encoding));

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_BEGIN);
  TEST_ASSERT(az_xml_parser_read_tag_begin(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("Error"), tag));
  TEST_ASSERT(parser.stack == 2);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_BEGIN);
  TEST_ASSERT(az_xml_parser_read_tag_begin(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("Code"), tag));
  TEST_ASSERT(parser.stack == 3);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_VALUE);
  TEST_ASSERT(az_xml_parser_read_value(&parser, &value) == AZ_OK);
  TEST_ASSERT(value.kind == AZ_XML_VALUE_STRING);
  TEST_ASSERT(az_span_eq(AZ_STR("InvalidQueryParameterValue"), value.data.string));
  TEST_ASSERT(parser.stack == 3);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_CLOSE);
  TEST_ASSERT(az_xml_parser_read_tag_close(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("Code"), tag));
  TEST_ASSERT(parser.stack == 2);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_BEGIN);
  TEST_ASSERT(az_xml_parser_read_tag_begin(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("Reason"), tag));
  TEST_ASSERT(parser.stack == 3);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_VALUE);
  TEST_ASSERT(az_xml_parser_read_value(&parser, &value) == AZ_OK);
  TEST_ASSERT(value.kind == AZ_XML_VALUE_STRING);
  TEST_ASSERT(az_span_eq(AZ_STR("invalid receipt format"), value.data.string));
  TEST_ASSERT(parser.stack == 3);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_CLOSE);
  TEST_ASSERT(az_xml_parser_read_tag_close(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("Reason"), tag));
  TEST_ASSERT(parser.stack == 2);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_BEGIN);
  TEST_ASSERT(az_xml_parser_read_tag_begin(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("TestEmptyConentTag"), tag));
  TEST_ASSERT(parser.stack == 2);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_TAG_CLOSE);
  TEST_ASSERT(az_xml_parser_read_tag_close(&parser, &tag) == AZ_OK);
  TEST_ASSERT(az_span_eq(AZ_STR("Error"), tag));
  TEST_ASSERT(parser.stack == 1);

  TEST_ASSERT(az_xml_parser_peek_next_item(&parser) == AZ_XML_STACK_EOF);

  TEST_ASSERT(true);
}
