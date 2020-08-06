// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

// Decode an input span from base64 to raw bytes
az_result sample_base64_decode(az_span base64_encoded, az_span in_span, az_span* out_span);

// Encode an input span of raw bytes to base64
az_result sample_base64_encode(az_span bytes, az_span in_span, az_span* out_span);

// HMAC256 sign an input span with an input key
az_result sample_hmac_sha256_sign(az_span key, az_span bytes, az_span in_span, az_span* out_span);
