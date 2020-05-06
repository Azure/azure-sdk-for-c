// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <az_span_internal.h>
#include <az_test_precondition.h>

#include <stdarg.h>
#include <stddef.h>

#include <limits.h>
#include <setjmp.h>
#include <stdint.h>

#include <cmocka.h>

#include <_az_cfg.h>

static void test_url_encode_basic(void** state)
{
  (void)state;
  {
    // Empty (null) input span, empty non-null output span.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer0 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 0);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer0, AZ_SPAN_NULL, &url_length)));
    assert_int_equal(url_length, 0);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("********************")));
  }
  {
    // Empty (non-null) input span, empty non-null output span.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer0 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 0);

    uint8_t buf1[1] = { 'A' };
    az_span const buffer0input = az_span_slice(AZ_SPAN_FROM_BUFFER(buf1), 0, 0);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer0, buffer0input, &url_length)));
    assert_int_equal(url_length, 0);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("********************")));
  }
  {
    // Just enough to succeed, but not percent-encode.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer5 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 5);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer5, AZ_SPAN_FROM_STR("AbCdE"), &url_length)));

    assert_int_equal(url_length, sizeof("AbCdE"));
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("AbCdE***************")));
  }
  {
    // Percent-encode single character.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer7 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 7);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer7, AZ_SPAN_FROM_STR("aBc/g"), &url_length)));

    assert_int_equal(url_length, sizeof("aBc%2Fg"));
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("aBc%2Fg*************")));
  }
  {
    // Could've been enough space to encode, but the character needs percent-encoding.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer2 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 9);

    int32_t url_length = 0xFF;
    assert_true(
        _az_span_url_encode(buffer2, AZ_SPAN_FROM_STR("/"), &url_length)
        == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);

    assert_int_equal(url_length, 0);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("********************")));
  }
  {
    // Single character needs encoding, and there's enough space.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer3 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 3);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer3, AZ_SPAN_FROM_STR("/"), &url_length)));

    assert_int_equal(url_length, sizeof("%2F"));
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("%2F*****************")));
  }
  {
    // Enough space to encode 3 characters, regardless of input.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer9 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 9);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer9, AZ_SPAN_FROM_STR("///"), &url_length)));

    assert_int_equal(url_length, sizeof("%2F%2F%2F"));
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("%2F%2F%2F***********")));
  }
  {
    // More than enough space to encode 3 characters, regardless of input.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer11 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 11);

    int32_t url_length = 0xFF;
    assert_true(az_succeeded(_az_span_url_encode(buffer11, AZ_SPAN_FROM_STR("///"), &url_length)));

    assert_int_equal(url_length, sizeof("%2F%2F%2F"));
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("%2F%2F%2F***********")));
  }
  {
    // Could've been enough space to encode 3 characters, but there's only space for two.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer10 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 11);

    int32_t url_length = 0xFF;
    assert_true(
        _az_span_url_encode(buffer10, AZ_SPAN_FROM_STR("AbC///"), &url_length)
        == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);

    assert_int_equal(url_length, 0);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("AbC%2F%2F**************")));
  }
  {
    // Could've been enough space to encode 3 characters, but there's only space for two
    // Slightly bigger buffer, still not big enough.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer11 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 11);

    int32_t url_length = 0xFF;
    assert_true(
        _az_span_url_encode(buffer11, AZ_SPAN_FROM_STR("AbC///"), &url_length)
        == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);

    assert_int_equal(url_length, 0);
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("AbC%2F%2F**************")));
  }
  {
    // Could've been enough space to encode 3 characters, and there's just enough space.
    uint8_t buf20[20] = {
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
      '*', '*', '*', '*', '*', '*', '*', '*', '*', '*',
    };

    az_span const buffer12 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf20), 0, 11);

    int32_t url_length = 0xFF;
    assert_true(
        az_succeeded(_az_span_url_encode(buffer12, AZ_SPAN_FROM_STR("AbC///"), &url_length)));

    assert_int_equal(url_length, sizeof("AbC%2F%2F%2F"));
    assert_true(az_span_is_content_equal(
        AZ_SPAN_FROM_BUFFER(buf20), AZ_SPAN_FROM_STR("AbC%2F%2F%2F***********")));
  }
}

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()
#endif

static void test_url_encode_preconditions(void** state)
{
  (void)state;
#ifdef AZ_NO_PRECONDITION_CHECKING
  {
    {
      // URL encode could never succeed.
      uint8_t buf5[5] = = { '*', '*', '*', '*', '*' };
      az_span const buffer5 = AZ_SPAN_FROM_BUFFER(buf5);

      int32_t url_length = 0xFF;
      assert_true(
          _az_span_url_encode(buffer5, AZ_SPAN_FROM_STR("1234567890"), &url_length)
          == AZ_ERROR_INSUFFICIENT_SPAN_SIZE);

      assert_int_equal(url_length, 0);
      assert_true(az_span_is_content_equal(buffer5, AZ_SPAN_FROM_STR("*****")));
    }
    {
      // Inut is empty, so the output is also empty BUT the output span is null.
      int32_t url_length = 0xFF;
      assert_true(az_succeeded(_az_span_url_encode(AZ_SPAN_NULL, AZ_SPAN_NULL, &url_length)));
      assert_int_equal(url_length, 0);
    }
    { // Overlapping buffers, same pointer.
      uint8_t buf[13] = { 'a', 'B', 'c', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*' };
      az_span const in_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 3);
      az_span const out_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 3);

      int32_t url_length = 0xFF;
      assert_true(az_succeeded(_az_span_url_encode(in_buffer, out_buffer, &url_length)));
      assert_int_equal(url_length, sizeof("aBc"));
      assert_true(
          az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf), AZ_SPAN_FROM_STR("aBc**********")));
    }
    {
      // Overlapping buffers, different pointers.
      uint8_t buf[13] = { 'a', 'B', 'c', '/', '/', '/', '*', '*', '*', '*', '*', '*', '*' };

      az_span const in_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 6);
      az_span const out_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 1, 13);

      int32_t url_length = 0xFF;
      assert_true(az_succeeded(_az_span_url_encode(in_buffer, out_buffer, &url_length)));
      assert_int_equal(url_length, sizeof("aaaaaa"));
      assert_true(
          az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf), AZ_SPAN_FROM_STR("aaaaaaa******")));
    }
    {
      // Overlapping buffers, writing before reading.
      uint8_t buf[12] = { '/', '/', '/', '/', '*', '*', '*', '*', '*', '*', '*', '*' };
      az_span const in_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 4);
      az_span const out_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 12);

      int32_t url_length = 0xFF;
      assert_true(az_succeeded(_az_span_url_encode(in_buffer, out_buffer, &url_length)));
      assert_int_equal(url_length, sizeof("%2F2F"));
      assert_true(
          az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf), AZ_SPAN_FROM_STR("%2F2F*******")));
    }
  }
#else
  {
    SETUP_PRECONDITION_CHECK_TESTS();

    {
      // URL encode could never succeed.
      uint8_t buf5[5] = { '*', '*', '*', '*', '*' };
      az_span const buffer5 = AZ_SPAN_FROM_BUFFER(buf5);

      int32_t url_length = 0xFF;
      ASSERT_PRECONDITION_CHECKED(
          _az_span_url_encode(buffer5, AZ_SPAN_FROM_STR("1234567890"), &url_length));

      assert_int_equal(url_length, 0xFF);
      assert_true(az_span_is_content_equal(buffer5, AZ_SPAN_FROM_STR("*****")));
    }

    {
      // Inut is empty, so the output is also empty BUT the output span is null.
      int32_t url_length = 0xFF;
      ASSERT_PRECONDITION_CHECKED(_az_span_url_encode(AZ_SPAN_NULL, AZ_SPAN_NULL, &url_length));
      assert_int_equal(url_length, 0xFF);
    }
    {
      // Overlapping buffers, same pointer.
      uint8_t buf[13] = { 'a', 'B', 'c', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*' };
      az_span const in_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 3);
      az_span const out_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 3);

      int32_t url_length = 0xFF;
      ASSERT_PRECONDITION_CHECKED(_az_span_url_encode(in_buffer, out_buffer, &url_length));
      assert_int_equal(url_length, 0xFF);
      assert_true(
          az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf), AZ_SPAN_FROM_STR("aBc**********")));
    }
    {
      // Overlapping buffers, different pointers.
      uint8_t buf[13] = { 'a', 'B', 'c', '/', '/', '/', '*', '*', '*', '*', '*', '*', '*' };
      az_span const in_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 6);
      az_span const out_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 1, 13);

      int32_t url_length = 0xFF;
      ASSERT_PRECONDITION_CHECKED(_az_span_url_encode(in_buffer, out_buffer, &url_length));
      assert_int_equal(url_length, 0xFF);
      assert_true(
          az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf), AZ_SPAN_FROM_STR("aBc///*******")));
    }
    {
      // Overlapping buffers, writing before reading.
      uint8_t buf[14] = { '/', '/', '/', '/', '*', '*', '*', '*', '*', '*', '*', '*', '*', '*' };
      az_span const in_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 0, 4);
      az_span const out_buffer = az_span_slice(AZ_SPAN_FROM_BUFFER(buf), 1, 14);

      int32_t url_length = 0xFF;
      ASSERT_PRECONDITION_CHECKED(_az_span_url_encode(in_buffer, out_buffer, &url_length));
      assert_int_equal(url_length, 0xFF);
      assert_true(
          az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf), AZ_SPAN_FROM_STR("////**********")));
    }
    {
      // NULL out_size parameter.
      uint8_t buf1[1] = { '*' };
      az_span const buffer0 = az_span_slice(AZ_SPAN_FROM_BUFFER(buf1), 0, 0);
      ASSERT_PRECONDITION_CHECKED(_az_span_url_encode(buffer0, AZ_SPAN_NULL, NULL));
      assert_true(az_span_is_content_equal(AZ_SPAN_FROM_BUFFER(buf1), AZ_SPAN_FROM_STR("*")));
    }
  }
#endif // AZ_NO_PRECONDITION_CHECKING
}

static void test_url_encode_usage(void** state)
{
  (void)state;

  // Typical use case.
  uint8_t buf[100] = { 0 };
  az_span const buffer = AZ_SPAN_FROM_BUFFER(buf);

  int32_t url_length = 0xFF;
  assert_true(az_succeeded(
      _az_span_url_encode(buffer, AZ_SPAN_FROM_STR("https://vault.azure.net"), &url_length)));

  assert_true(az_span_is_content_equal(
      az_span_slice(buffer, 0, url_length), AZ_SPAN_FROM_STR("https%3A%2F%2Fvault.azure.net")));

  assert_int_equal(url_length, sizeof("https%3A%2F%2Fvault.azure.net"));
}

static void test_url_encode_full(void** state)
{
  (void)state;

  // Go through all 256 values.
  uint8_t values256[256] = {
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
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
  };

  uint8_t buf[256 * 3] = { 0 };
  az_span const buffer = AZ_SPAN_FROM_BUFFER(buf);

  int32_t url_length = 0xFF;
  assert_true(
      az_succeeded(_az_span_url_encode(buffer, AZ_SPAN_FROM_BUFFER(values256), &url_length)));

  int const nunreserved = sizeof("-_.~"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz"
                                 "0123456789");

  assert_int_equal(url_length, nunreserved + (256 - nunreserved) * 3);

  assert_true(az_span_is_content_equal(
      buffer,
      AZ_SPAN_FROM_STR("%00%01%02%03%04%05%06%07%08%09%0A%0B%0C%0D%0E%0F"
                       "%10%11%12%13%14%15%16%17%18%19%1A%1B%1C%1D%1E%1F"
                       "%20%21%22%23%24%25%26%27%28%29%2A%2B%2C-.%2F"
                       "0123456789%3A%3B%3C%3D%3E%3F"
                       "%40ABCDEFGHIJKLMNO"
                       "PQRSTUVWXYZ%5B%5C%5D%5E_"
                       "%60abcdefghijklmno"
                       "pqrstuvwxyz%7B%7C%7D~%7F"
                       "%80%81%82%83%84%85%86%87%88%89%8A%8B%8C%8D%8E%8F"
                       "%90%91%92%93%94%95%96%97%98%99%9A%9B%9C%9D%9E%9F"
                       "%A0%A1%A2%A3%A4%A5%A6%A7%A8%A9%AA%AB%AC%AD%AE%AF"
                       "%B0%B1%B2%B3%B4%B5%B6%B7%B8%B9%BA%BB%BC%BD%BE%BF"
                       "%C0%C1%C2%C3%C4%C5%C6%C7%C8%C9%CA%CB%CC%CD%CE%CF"
                       "%D0%D1%D2%D3%D4%D5%D6%D7%D8%D9%DA%DB%DC%DD%DE%DF"
                       "%E0%E1%E2%E3%E4%E5%E6%E7%E8%E9%EA%EB%EC%ED%EE%EF"
                       "%F0%F1%F2%F3%F4%F5%F6%F7%F8%F9%FA%FB%FC%FD%FE%FF")));
}

int test_az_url_encode()
{
  struct CMUnitTest const tests[] = {
    cmocka_unit_test(test_url_encode_basic),
    cmocka_unit_test(test_url_encode_preconditions),
    cmocka_unit_test(test_url_encode_usage),
    cmocka_unit_test(test_url_encode_full),
  };

  return cmocka_run_group_tests_name("az_core_encode", tests, NULL, NULL);
}
