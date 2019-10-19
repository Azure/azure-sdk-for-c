// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_base64.h>
#include <az_contract.h>

#include <assert.h>

#include <_az_cfg.h>

enum {
  TRIBYTE_OCTETS = 3,
  TRIBYTE_SEXTETS = 4,

  TRIBYTE_OCTET0_INDEX = 0,
  TRIBYTE_OCTET1_INDEX = 1,
  TRIBYTE_OCTET2_INDEX = 2,

  TRIBYTE_SEXTET0_INDEX = 0,
  TRIBYTE_SEXTET1_INDEX = 1,
  TRIBYTE_SEXTET2_INDEX = 2,
  TRIBYTE_SEXTET3_INDEX = 3,

  TRIBYTE_OCTET0_SEXTET0_MASK = 0xFC,
  TRIBYTE_OCTET0_SEXTET1_MASK = 0x03,
  TRIBYTE_OCTET1_SEXTET1_MASK = 0xF0,
  TRIBYTE_OCTET1_SEXTET2_MASK = 0x0F,
  TRIBYTE_OCTET2_SEXTET2_MASK = 0xC0,
  TRIBYTE_OCTET2_SEXTET3_MASK = 0x3F,

  TRIBYTE_OCTET0_SEXTET0_RSHIFT = 2,
  TRIBYTE_OCTET0_SEXTET1_LSHIFT = 4,
  TRIBYTE_OCTET1_SEXTET1_RSHIFT = 4,
  TRIBYTE_OCTET1_SEXTET2_LSHIFT = 2,
  TRIBYTE_OCTET2_SEXTET2_RSHIFT = 6,
};

enum {
  BASE64_RANGE1_MIN = 'A',
  BASE64_RANGE1_MAX = 'Z',
  BASE64_RANGE1_START = 0,

  BASE64_RANGE2_MIN = 'a',
  BASE64_RANGE2_MAX = 'z',
  BASE64_RANGE2_START = BASE64_RANGE1_START + (BASE64_RANGE1_MAX - BASE64_RANGE1_MIN) + 1,

  BASE64_RANGE3_MIN = '0',
  BASE64_RANGE3_MAX = '9',
  BASE64_RANGE3_START = BASE64_RANGE2_START + (BASE64_RANGE2_MAX - BASE64_RANGE2_MIN) + 1,

  BASE64_CHAR63 = '+',
  BASE64_CHAR63_URL = '-',
  BASE64_CHAR63_INDEX = BASE64_RANGE3_START + (BASE64_RANGE3_MAX - BASE64_RANGE3_MIN) + 1,

  BASE64_CHAR64 = '/',
  BASE64_CHAR64_URL = '_',
  BASE64_CHAR64_INDEX = BASE64_CHAR63_INDEX + 1,

  BASE64_INVALID_VALUE = (uint8_t)~0,
  BASE64_PADDING_CHAR = '=',
};

AZ_STATIC_ASSERT(BASE64_CHAR64_INDEX == 63)

AZ_INLINE uint8_t uint6_as_base64(bool const base64url, uint8_t const uint6) {
  if (BASE64_RANGE1_START <= uint6 && uint6 < BASE64_RANGE2_START) {
    return BASE64_RANGE1_MIN + uint6 - BASE64_RANGE1_START;
  } else if (BASE64_RANGE2_START <= uint6 && uint6 < BASE64_RANGE3_START) {
    return BASE64_RANGE2_MIN + uint6 - BASE64_RANGE2_START;
  } else if (BASE64_RANGE3_START <= uint6 && uint6 < BASE64_CHAR63_INDEX) {
    return BASE64_RANGE3_MIN + uint6 - BASE64_RANGE3_START;
  } else {
    switch (uint6) {
      case BASE64_CHAR63_INDEX:
        return base64url ? BASE64_CHAR63_URL : BASE64_CHAR63;
      case BASE64_CHAR64_INDEX:
        return base64url ? BASE64_CHAR64_URL : BASE64_CHAR64;
      default:
        return BASE64_PADDING_CHAR;
    }
  }
}

AZ_INLINE uint8_t base64_as_uint6(uint8_t const base64) {
  if (BASE64_RANGE1_MIN <= base64 && base64 <= BASE64_RANGE1_MAX) {
    return BASE64_RANGE1_START + (base64 - BASE64_RANGE1_MIN);
  } else if (BASE64_RANGE2_MIN <= base64 && base64 <= BASE64_RANGE2_MAX) {
    return BASE64_RANGE2_START + (base64 - BASE64_RANGE2_MIN);
  } else if (BASE64_RANGE3_MIN <= base64 && base64 <= BASE64_RANGE3_MAX) {
    return BASE64_RANGE3_START + (base64 - BASE64_RANGE3_MIN);
  } else {
    switch (base64) {
      case BASE64_CHAR63:
      case BASE64_CHAR63_URL:
        return BASE64_CHAR63_INDEX;
      case BASE64_CHAR64:
      case BASE64_CHAR64_URL:
        return BASE64_CHAR64_INDEX;
      default:
        return (uint8_t)BASE64_INVALID_VALUE;
    }
  }
}

az_result az_base64_encode(
    bool const base64url,
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);
  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(input)) {
    return AZ_ERROR_ARG;
  }

  size_t const result_size = (TRIBYTE_SEXTETS * (input.size / TRIBYTE_OCTETS))
      + ((input.size % TRIBYTE_OCTETS == 0)
             ? 0
             : (base64url ? 1 + (input.size % TRIBYTE_OCTETS) : TRIBYTE_SEXTETS));

  az_const_span const result = (az_const_span){ .begin = buffer.begin, .size = result_size };

  if (az_const_span_is_overlap(input, result)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < result_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  for (size_t oct = 0, sxt = 0; oct < input.size; oct += TRIBYTE_OCTETS, sxt += TRIBYTE_SEXTETS) {
    size_t const nsextets
        = ((result_size - sxt) < TRIBYTE_SEXTETS) ? (result_size - sxt) : TRIBYTE_SEXTETS;

    uint8_t octet1 = 0;
    if (nsextets >= TRIBYTE_SEXTETS - 1) {
      if (oct + TRIBYTE_OCTET1_INDEX < input.size) {
        assert(oct + TRIBYTE_OCTET1_INDEX < input.size);
        octet1 = input.begin[oct + TRIBYTE_OCTET1_INDEX];
      }

      uint8_t octet2 = 0;
      if (nsextets == TRIBYTE_SEXTETS) {
        assert(sxt + TRIBYTE_SEXTET3_INDEX < result_size);

        if (oct + 2 < input.size) {
          assert(oct + TRIBYTE_OCTET2_INDEX < input.size);

          octet2 = input.begin[oct + TRIBYTE_OCTET2_INDEX];

          buffer.begin[sxt + TRIBYTE_SEXTET3_INDEX]
              = uint6_as_base64(base64url, octet2 & TRIBYTE_OCTET2_SEXTET3_MASK);
        } else {
          buffer.begin[sxt + TRIBYTE_SEXTET3_INDEX] = BASE64_PADDING_CHAR;
        }
      }

      assert(sxt + TRIBYTE_SEXTET2_INDEX < result_size);
      buffer.begin[sxt + TRIBYTE_SEXTET2_INDEX] = (oct + TRIBYTE_OCTET1_INDEX < input.size)
          ? uint6_as_base64(
              base64url,
              ((octet1 & TRIBYTE_OCTET1_SEXTET2_MASK) << TRIBYTE_OCTET1_SEXTET2_LSHIFT)
                  | ((octet2 & TRIBYTE_OCTET2_SEXTET2_MASK) >> TRIBYTE_OCTET2_SEXTET2_RSHIFT))
          : BASE64_PADDING_CHAR;
    }

    assert(sxt + TRIBYTE_SEXTET1_INDEX < result_size);
    assert(oct + TRIBYTE_OCTET0_INDEX < input.size);
    buffer.begin[sxt + TRIBYTE_SEXTET1_INDEX] = uint6_as_base64(
        base64url,
        ((input.begin[oct + TRIBYTE_OCTET0_INDEX] & TRIBYTE_OCTET0_SEXTET1_MASK)
         << TRIBYTE_OCTET0_SEXTET1_LSHIFT)
            | ((octet1 & TRIBYTE_OCTET1_SEXTET1_MASK) >> TRIBYTE_OCTET1_SEXTET1_RSHIFT));

    assert(sxt + TRIBYTE_SEXTET0_INDEX < result_size);
    assert(oct + TRIBYTE_OCTET0_INDEX < input.size);
    buffer.begin[sxt + TRIBYTE_SEXTET0_INDEX] = uint6_as_base64(
        base64url,
        (input.begin[oct + TRIBYTE_OCTET0_INDEX] & TRIBYTE_OCTET0_SEXTET0_MASK)
            >> TRIBYTE_OCTET0_SEXTET0_RSHIFT);
  }

  *out_result = result;
  return AZ_OK;
}

az_result az_base64_decode(
    az_span const buffer,
    az_const_span const input,
    az_const_span * const out_result) {
  AZ_CONTRACT_ARG_NOT_NULL(out_result);

  if (!az_span_is_valid(buffer) || !az_const_span_is_valid(input)) {
    return AZ_ERROR_ARG;
  }

  size_t padding = 0;
  for (size_t ri = input.size; ri > 0; --ri) {
    if (input.begin[ri - 1] == BASE64_PADDING_CHAR) {
      ++padding;
    } else {
      for (; ri > 0; --ri) {
        if (base64_as_uint6(input.begin[ri - 1]) == BASE64_INVALID_VALUE) {
          return AZ_ERROR_ARG;
        }
      }
      break;
    }
  }

  size_t const input_size = input.size - padding;

  size_t const remainder = input_size % TRIBYTE_SEXTETS;
  if (remainder == 1) {
    return AZ_ERROR_ARG;
  }

  size_t const result_size
      = (TRIBYTE_OCTETS * (input_size / TRIBYTE_SEXTETS)) + (remainder == 0 ? 0 : remainder - 1);

  az_const_span const result = (az_const_span){ .begin = buffer.begin, .size = result_size };

  if (az_const_span_is_overlap(input, result)) {
    return AZ_ERROR_ARG;
  }

  if (buffer.size < result_size) {
    return AZ_ERROR_BUFFER_OVERFLOW;
  }

  for (size_t oct = 0, sxt = 0; oct < result_size; oct += TRIBYTE_OCTETS, sxt += TRIBYTE_SEXTETS) {
    size_t const noctets
        = ((result_size - oct) < TRIBYTE_OCTETS) ? (result_size - oct) : TRIBYTE_OCTETS;

    assert(sxt + TRIBYTE_SEXTET1_INDEX < input_size);
    uint8_t const sextet1 = base64_as_uint6(input.begin[sxt + TRIBYTE_SEXTET1_INDEX]);

    if (noctets >= TRIBYTE_OCTETS - 1) {
      assert(sxt + TRIBYTE_SEXTET2_INDEX < input_size);
      assert(oct + TRIBYTE_OCTET1_INDEX < result_size);

      uint8_t const sextet2 = base64_as_uint6(input.begin[sxt + TRIBYTE_SEXTET2_INDEX]);

      buffer.begin[oct + TRIBYTE_OCTET1_INDEX]
          = (sextet1 << TRIBYTE_OCTET1_SEXTET1_RSHIFT) | (sextet2 >> TRIBYTE_OCTET1_SEXTET2_LSHIFT);

      if (noctets == TRIBYTE_OCTETS) {
        assert(oct + TRIBYTE_OCTET2_INDEX < result_size);
        assert(sxt + TRIBYTE_SEXTET3_INDEX < input_size);

        buffer.begin[oct + TRIBYTE_OCTET2_INDEX] = (sextet2 << TRIBYTE_OCTET2_SEXTET2_RSHIFT)
            | base64_as_uint6(input.begin[sxt + TRIBYTE_SEXTET3_INDEX]);
      }
    }

    assert(oct + TRIBYTE_OCTET0_INDEX < result_size);
    assert(sxt + TRIBYTE_SEXTET0_INDEX < input_size);

    buffer.begin[oct + TRIBYTE_OCTET0_INDEX]
        = (base64_as_uint6(input.begin[sxt + TRIBYTE_SEXTET0_INDEX])
           << TRIBYTE_OCTET0_SEXTET0_RSHIFT)
        | (sextet1 >> TRIBYTE_OCTET0_SEXTET1_LSHIFT);
  }

  *out_result = result;
  return AZ_OK;
}