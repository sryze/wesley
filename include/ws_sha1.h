/* Based on code from RFC 3174: https://tools.ietf.org/html/rfc3174 */

#ifndef WS_SHA1_H
#define WS_SHA1_H

#include "ws_types.h"

#define WS_SHA1_HASH_SIZE 20

enum {
    WS_SHA1_SUCCESS = 0,
    WS_SHA1_NULL,           /* Null pointer parameter */
    WS_SHA1_INPUT_TOO_LONG, /* input data too long */
    WS_SHA1_STATE_ERROR     /* called Input after Result */
};

typedef struct ws_sha1_context
{
    ws_uint32_t intermediate_hash[WS_SHA1_HASH_SIZE / 4]; /* Message Digest  */
    ws_uint32_t length_low;         /* Message length in bits          */
    ws_uint32_t length_high;        /* Message length in bits          */
                                    /* Index into message block array  */
    ws_int16_t message_block_index;
    ws_uint8_t message_block[64];   /* 512-bit message blocks          */
    int computed;                   /* Is the digest computed?         */
    int corrupted;                  /* Is the message digest corrupted? */
} ws_sha1_context;

int ws_sha1_reset(ws_sha1_context *context);
int ws_sha1_input(
  ws_sha1_context *context,
  const ws_uint8_t *message_array,
  unsigned length);
int ws_sha1_result(
  ws_sha1_context *contex,
  ws_uint8_t message_digest[WS_SHA1_HASH_SIZE]);

#endif /* WS_SHA1_H */
