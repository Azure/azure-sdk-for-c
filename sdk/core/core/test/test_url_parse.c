// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "./az_test.h"

#include <az_url_internal.h>

#include <_az_cfg.h>

void test_url_parse() {
  {
    az_url url = { 0 };
    TEST_ASSERT(
        az_url_parse(
            AZ_SPAN_FROM_STR("https://someone@example.com:81/folder/folder?a=b&c=d#Header"), &url)
        == AZ_OK);
    TEST_ASSERT(az_span_is_equal(url.scheme, AZ_SPAN_FROM_STR("https")));
    TEST_ASSERT(az_span_is_equal(url.authority.userinfo, AZ_SPAN_FROM_STR("someone")));
    TEST_ASSERT(az_span_is_equal(url.authority.host, AZ_SPAN_FROM_STR("example.com")));
    {
      az_span host = url.authority.host;
      az_span domain = { 0 };
      TEST_ASSERT(az_host_read_domain(&host, &domain) == AZ_OK);
      TEST_ASSERT(az_span_is_equal(domain, AZ_SPAN_FROM_STR("com")));
      TEST_ASSERT(az_span_is_equal(host, AZ_SPAN_FROM_STR("example")));
      TEST_ASSERT(az_host_read_domain(&host, &domain) == AZ_OK);
      TEST_ASSERT(az_span_is_equal(domain, AZ_SPAN_FROM_STR("example")));
      TEST_ASSERT(az_span_is_equal(host, AZ_SPAN_FROM_STR("")));
      TEST_ASSERT(az_host_read_domain(&host, &domain) == AZ_ERROR_ITEM_NOT_FOUND);
    }
    TEST_ASSERT(az_span_is_equal(url.authority.port, AZ_SPAN_FROM_STR("81")));
    TEST_ASSERT(az_span_is_equal(url.path, AZ_SPAN_FROM_STR("/folder/folder")));
    TEST_ASSERT(az_span_is_equal(url.query, AZ_SPAN_FROM_STR("a=b&c=d")));
    TEST_ASSERT(az_span_is_equal(url.fragment, AZ_SPAN_FROM_STR("Header")));
  }
  {
    az_url url = { 0 };
    TEST_ASSERT(az_url_parse(AZ_SPAN_FROM_STR("https://example.com.localhost?a=b"), &url) == AZ_OK);
    TEST_ASSERT(az_span_is_equal(url.scheme, AZ_SPAN_FROM_STR("https")));
    TEST_ASSERT(az_span_is_equal(url.authority.userinfo, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(az_span_is_equal(url.authority.host, AZ_SPAN_FROM_STR("example.com.localhost")));
    TEST_ASSERT(az_span_is_equal(url.authority.port, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(az_span_is_equal(url.path, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(az_span_is_equal(url.query, AZ_SPAN_FROM_STR("a=b")));
    TEST_ASSERT(az_span_is_equal(url.fragment, AZ_SPAN_FROM_STR("")));
  }
  {
    az_url url = { 0 };
    TEST_ASSERT(
        az_url_parse(AZ_SPAN_FROM_STR("https://sub-domain.example.com/someone#a=b"), &url)
        == AZ_OK);
    TEST_ASSERT(az_span_is_equal(url.scheme, AZ_SPAN_FROM_STR("https")));
    TEST_ASSERT(az_span_is_equal(url.authority.userinfo, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(az_span_is_equal(url.authority.host, AZ_SPAN_FROM_STR("sub-domain.example.com")));
    TEST_ASSERT(az_span_is_equal(url.authority.port, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(az_span_is_equal(url.path, AZ_SPAN_FROM_STR("/someone")));
    TEST_ASSERT(az_span_is_equal(url.query, AZ_SPAN_FROM_STR("")));
    TEST_ASSERT(az_span_is_equal(url.fragment, AZ_SPAN_FROM_STR("a=b")));
  }
}
