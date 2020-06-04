// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "az_span.h"
#include "sample_base64.h"

// Decode an input span from base64 to bytes
az_result sample_base64_decode(az_span base64_encoded, az_span in_span, az_span* out_span)
{
  az_result result;

  BIO* b64_decoder;
  BIO* source_mem_bio;

  memset(az_span_ptr(in_span), 0, (size_t)az_span_size(in_span));

  // Create a BIO filter to process the bytes
  b64_decoder = BIO_new(BIO_f_base64());

  // Get the source BIO to push through the filter
  source_mem_bio = BIO_new_mem_buf(az_span_ptr(base64_encoded), (size_t)az_span_size(base64_encoded));

  // Push the memory throught the filter
  source_mem_bio = BIO_push(b64_decoder, source_mem_bio);

  // Set flags to not have a newline and close the BIO
  BIO_set_flags(source_mem_bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_set_close(source_mem_bio, BIO_CLOSE);

  // Read the memory which was pushed through the filter
  int read_data = BIO_read(source_mem_bio, az_span_ptr(in_span), az_span_size(in_span));

  // Set the output span
  if (read_data > 0)
  {
    *out_span = az_span_init(az_span_ptr(in_span), (int32_t)read_data);
    result = AZ_OK;
  }
  else
  {
    result = AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  // Free the BIO chain
  BIO_free_all(source_mem_bio);

  return result;
}

// Encode an input span of bytes to base64
az_result sample_base64_encode(az_span bytes, az_span in_span, az_span* out_span)
{
  az_result result;

  BIO* sink_mem_bio;
  BIO* b64_encoder;
  BUF_MEM* encoded_mem_ptr;

  // Create a BIO filter to process the bytes
  b64_encoder = BIO_new(BIO_f_base64());

  // Create a memory sink BIO to process bytes to
  sink_mem_bio = BIO_new(BIO_s_mem());

  // Push the sink to the encoder
  b64_encoder = BIO_push(b64_encoder, sink_mem_bio);

  //Set no newline flag for the encoder
  BIO_set_flags(b64_encoder, BIO_FLAGS_BASE64_NO_NL);

  // Write the bytes to be encoded
  BIO_write(b64_encoder, az_span_ptr(bytes), (int)az_span_size(bytes));

  // Flush the BIO
  BIO_flush(b64_encoder);

  // Get the pointer to the encoded bytes
  BIO_get_mem_ptr(b64_encoder, &encoded_mem_ptr);

  if ((size_t)az_span_size(in_span) >= encoded_mem_ptr->length)
  {
    // Copy the bytes to the output and initialize output span
    memcpy(az_span_ptr(in_span), encoded_mem_ptr->data, encoded_mem_ptr->length);
    *out_span = az_span_init(az_span_ptr(in_span), (int32_t)encoded_mem_ptr->length);

    result = AZ_OK;
  }
  else
  {
    result = AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  BIO_free_all(b64_encoder);

  return result;
}

// HMAC256 an input span with an input key
az_result sample_hmac_encrypt(az_span key, az_span bytes, az_span in_span, az_span* out_span)
{
  az_result result;

  unsigned int hmac_encode_len;
  unsigned char* hmac = HMAC(
      EVP_sha256(),
      (void*)az_span_ptr(key),
      az_span_size(key),
      az_span_ptr(bytes),
      (size_t)az_span_size(bytes),
      az_span_ptr(in_span),
      &hmac_encode_len);

  if (hmac != NULL)
  {
    *out_span = az_span_init(az_span_ptr(in_span), (int32_t)hmac_encode_len);
    result = AZ_OK;
  }
  else
  {
    result = AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  return result;
}
