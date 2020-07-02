/* Based on code from RFC 3174: https://tools.ietf.org/html/rfc3174 */

#include "ws_sha1.h"

#define CIRCULAR_SHIFT(bits, word) \
        (((word) << (bits)) | ((word) >> (32 - (bits))))

static void pad_message(ws_sha1_context *);
static void process_message_block(ws_sha1_context *);

int ws_sha1_reset(ws_sha1_context *context)
{
  if (!context) {
    return WS_SHA1_NULL;
  }

  context->length_low = 0;
  context->length_high = 0;
  context->message_block_index = 0;

  context->intermediate_hash[0] = 0x67452301;
  context->intermediate_hash[1] = 0xEFCDAB89;
  context->intermediate_hash[2] = 0x98BADCFE;
  context->intermediate_hash[3] = 0x10325476;
  context->intermediate_hash[4] = 0xC3D2E1F0;

  context->computed = 0;
  context->corrupted = 0;

  return WS_SHA1_SUCCESS;
}

int ws_sha1_result(ws_sha1_context *context,
                   ws_uint8_t message_digest[WS_SHA1_HASH_SIZE])
{
  int i;

  if (!context || !message_digest) {
    return WS_SHA1_NULL;
  }

  if (context->corrupted) {
    return context->corrupted;
  }

  if (!context->computed) {
    pad_message(context);
    for (i = 0; i < 64; i++) {
      /* message may be sensitive, clear it out */
      context->message_block[i] = 0;
    }
    context->length_low = 0;  /* and clear length */
    context->length_high = 0;
    context->computed = 1;
  }

  for (i = 0; i < WS_SHA1_HASH_SIZE; i++) {
    message_digest[i] = context->intermediate_hash[i >> 2]
              >> 8 * (3 - (i & 0x03));
  }

  return WS_SHA1_SUCCESS;
}

int ws_sha1_input(ws_sha1_context *context,
                  const ws_uint8_t *message_array,
                  unsigned length)
{
  if (!length) {
    return WS_SHA1_SUCCESS;
  }

  if (!context || !message_array) {
    return WS_SHA1_NULL;
  }

  if (context->computed) {
    context->corrupted = WS_SHA1_STATE_ERROR;
    return WS_SHA1_STATE_ERROR;
  }

  if (context->corrupted) {
    return context->corrupted;
  }

  while (length-- && !context->corrupted) {
    context->message_block[context->message_block_index++] =
      (*message_array & 0xFF);

    context->length_low += 8;
    if (context->length_low == 0) {
      context->length_high++;
      if (context->length_high == 0) {
        /* Message is too long */
        context->corrupted = 1;
      }
    }

    if (context->message_block_index == 64) {
      process_message_block(context);
    }

    message_array++;
  }

  return WS_SHA1_SUCCESS;
}

static void process_message_block(ws_sha1_context *context)
{
  const ws_uint32_t K[] = {  /* Constants defined in SHA-1 */
    0x5A827999,
    0x6ED9EBA1,
    0x8F1BBCDC,
    0xCA62C1D6
  };
  int t;                     /* Loop counter */
  ws_uint32_t temp;          /* Temporary word value */
  ws_uint32_t W[80];         /* Word sequence */
  ws_uint32_t A, B, C, D, E; /* Word buffers */

  /*
   * Initialize the first 16 words in the array
   */
  for (t = 0; t < 16; t++) {
    W[t] = context->message_block[t * 4] << 24;
    W[t] |= context->message_block[t * 4 + 1] << 16;
    W[t] |= context->message_block[t * 4 + 2] << 8;
    W[t] |= context->message_block[t * 4 + 3];
  }

  for (t = 16; t < 80; t++) {
     W[t] = CIRCULAR_SHIFT(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
  }

  A = context->intermediate_hash[0];
  B = context->intermediate_hash[1];
  C = context->intermediate_hash[2];
  D = context->intermediate_hash[3];
  E = context->intermediate_hash[4];

  for (t = 0; t < 20; t++) {
    temp = CIRCULAR_SHIFT(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
    E = D;
    D = C;
    C = CIRCULAR_SHIFT(30,B);
    B = A;
    A = temp;
  }

  for (t = 20; t < 40; t++) {
    temp = CIRCULAR_SHIFT(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
    E = D;
    D = C;
    C = CIRCULAR_SHIFT(30,B);
    B = A;
    A = temp;
  }

  for (t = 40; t < 60; t++) {
    temp = CIRCULAR_SHIFT(5,A) +
      ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
    E = D;
    D = C;
    C = CIRCULAR_SHIFT(30,B);
    B = A;
    A = temp;
  }

  for (t = 60; t < 80; t++) {
    temp = CIRCULAR_SHIFT(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
    E = D;
    D = C;
    C = CIRCULAR_SHIFT(30,B);
    B = A;
    A = temp;
  }

  context->intermediate_hash[0] += A;
  context->intermediate_hash[1] += B;
  context->intermediate_hash[2] += C;
  context->intermediate_hash[3] += D;
  context->intermediate_hash[4] += E;
  context->message_block_index = 0;
}

static void pad_message(ws_sha1_context *context)
{
  /*
   * Check to see if the current message block is too small to hold
   * the initial padding bits and length.  If so, we will pad the
   * block, process it, and then continue padding into a second
   * block.
   */
  if (context->message_block_index > 55) {
    context->message_block[context->message_block_index++] = 0x80;
    while (context->message_block_index < 64) {
      context->message_block[context->message_block_index++] = 0;
    }
    process_message_block(context);
    while (context->message_block_index < 56) {
      context->message_block[context->message_block_index++] = 0;
    }
  } else {
    context->message_block[context->message_block_index++] = 0x80;
    while (context->message_block_index < 56) {
      context->message_block[context->message_block_index++] = 0;
    }
  }

  /*
   * Store the message length as the last 8 octets
   */
  context->message_block[56] = context->length_high >> 24;
  context->message_block[57] = context->length_high >> 16;
  context->message_block[58] = context->length_high >> 8;
  context->message_block[59] = context->length_high;
  context->message_block[60] = context->length_low >> 24;
  context->message_block[61] = context->length_low >> 16;
  context->message_block[62] = context->length_low >> 8;
  context->message_block[63] = context->length_low;

  process_message_block(context);
}
