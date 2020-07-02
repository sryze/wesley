#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "ws.h"
#include "ws_base64.h"
#include "ws_http.h"
#include "ws_sha1.h"
#include "ws_socket.h"
#include "ws_string.h"

#define count_of(a) (sizeof(a) / sizeof(a[0]))
#define MAX_HTTP_HEADERS (8 * 1024) /* HTTP RFC recommends at least 8000 */
#define SHA1_HASH_SIZE 20
#define PAYLOAD_LENGTH_16 126
#define PAYLOAD_LENGTH_64 127

#ifndef _WIN32
  #define htonll(x) ((1==htonl(1)) ? (x) : ((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
  #define ntohll(x) ((1==ntohl(1)) ? (x) : ((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))
#endif

struct ws_http_handshake_state {
  bool has_upgrade_connection;
  bool upgrade_to_websocket;
  int websocket_version;
  const char *websocket_key;
  size_t websocket_key_length;
};

static const char
  ws_key_accept_magic[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static const char *ws_error_messages[] = {
  "Out of memory",
  "Error sending data",
  "Error receiving data",
  "Could not parse HTTP request",
  "Incorrect HTTP method in handshake request",
  "Unsupported version of HTTP protocol",
  "Unsupported version of WebSocket protocol",
  "Request does not include valid conneciton upgrade headers",
  "Request does not include \"Sec-WebSocket-Key\" header"
};

const char *ws_error_message(int error) {
  if (error <= 0 || error >= count_of(ws_error_messages)) {
    return "Unknown error";
  }
  return ws_error_messages[error];
}

static void on_handshake_header(const struct ws_http_fragment *name,
                                const struct ws_http_fragment *value,
                                void *data)
{
  struct ws_http_handshake_state *state = data;

  if (strncasecmp(name->ptr, "Connection", name->length) == 0) {
    if (strncasecmp(value->ptr, "Upgrade", value->length) == 0) {
      state->has_upgrade_connection = true;
    } else {
      char *values = strndup(value->ptr, value->length);
      char *src = values;
      char *tok_context = NULL;
      char *value;
      while ((value = strtok_r(src, ", \t", &tok_context)) != NULL) {
        if (strcasecmp(value, "Upgrade") == 0) {
          state->has_upgrade_connection = true;
          break;
        }
        src = NULL;
      }
      free(values);
    }
  } else if (strncasecmp(name->ptr, "Upgrade", name->length) == 0) {
    state->upgrade_to_websocket =
      strncasecmp(value->ptr, "websocket", value->length) == 0;
  } else if (strncasecmp(name->ptr,
                         "Sec-WebSocket-Version",
                         name->length) == 0) {
    state->websocket_version = atoin(value->ptr, value->length);
  } else if (strncasecmp(name->ptr,
                         "Sec-WebSocket-Key",
                         name->length) == 0) {
    state->websocket_key = value->ptr;
    state->websocket_key_length = value->length;
  }
}

static int ws_parse_connect_request(const char *buf,
                                    size_t len,
                                    const char **key,
                                    size_t *key_len)
{
  struct ws_http_handshake_state parse_state = {0};
  const char *result;
  struct ws_http_fragment http_method;
  int http_version;

  result = ws_http_parse_request_line(buf, &http_method, NULL, &http_version);
  if (result == NULL) {
    return WS_ERROR_HTTP_REQUEST;
  }

  result = ws_http_parse_headers(result, on_handshake_header, &parse_state);
  if (result == NULL) {
    return WS_ERROR_HTTP_REQUEST;
  }

  if (strncmp(http_method.ptr, "GET", http_method.length) != 0) {
    return WS_ERROR_HTTP_METHOD;
  } else if (http_version > 0x01FF) {
    return WS_ERROR_HTTP_VERSION;
  } else if (parse_state.websocket_version != WS_PROTOCOL_VERSION) {
    return WS_ERROR_WEBSOCKET_VERSION;
  } else if (!parse_state.has_upgrade_connection
             || !parse_state.upgrade_to_websocket) {
    return WS_ERROR_NO_UPGRADE;
  } else if (parse_state.websocket_key == NULL
             || strlen(parse_state.websocket_key) == 0) {
    return WS_ERROR_NO_KEY;
  }

  *key = parse_state.websocket_key;
  *key_len = parse_state.websocket_key_length;

  return 0;
}

int ws_send_handshake_accept(ws_socket_t sock, const char *key)
{
  size_t key_len;
  char *key_hash_input;
  ws_sha1_context sha1_context;
  char key_hash[SHA1_HASH_SIZE];
  char accept[SHA1_HASH_SIZE * 2];
  size_t accept_len;
  char response[256];

  key_len = strlen(key);
  key_hash_input =
    malloc(sizeof(*key_hash_input) * (key_len + sizeof(ws_key_accept_magic)));
  if (key_hash_input == NULL) {
    return WS_ERROR_MEMORY;
  }

  memcpy(key_hash_input, key, key_len);
  memcpy(key_hash_input + key_len,
         ws_key_accept_magic,
         sizeof(ws_key_accept_magic));

  ws_sha1_reset(&sha1_context);
  ws_sha1_input(&sha1_context,
                (ws_uint8_t *)key_hash_input,
                (unsigned int)strlen(key_hash_input));
  ws_sha1_result(&sha1_context, (ws_uint8_t *)key_hash);
  free(key_hash_input);

  accept_len = ws_base64_encode(key_hash,
                                sizeof(key_hash),
                                accept,
                                sizeof(accept) - 1);
  accept[accept_len] = '\0';

  snprintf(response, sizeof(response),
    "HTTP/1.1 101 Switching Protocols\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    "Sec-WebSocket-Accept: %s\r\n\r\n",
    accept);
  if (ws_socket_send_string(sock, response) < 0) {
    return WS_ERROR_SEND;
  }

  return 0;
}

int ws_accept(ws_socket_t sock)
{
  int error;
  char buf[MAX_HTTP_HEADERS] = {0};
  int len;
  const char *key;
  size_t key_len;
  char *key_copy;

  len = ws_http_recv_headers(sock, buf, sizeof(buf));
  if (len <= 0) {
    return WS_ERROR_RECV;
  }

  error = ws_parse_connect_request(buf, (size_t)len, &key, &key_len);
  if (error != 0) {
    return error;
  }

  key_copy = strndup(key, key_len);
  if (key_copy == NULL) {
    return WS_ERROR_MEMORY;
  }

  error = ws_send_handshake_accept(sock, key_copy);
  free(key_copy);
  if (error != 0) {
    return error;
  }

  return 0;
}

static ws_uint8_t *ws_alloc_frame(ws_uint16_t flags,
                                  ws_uint8_t opcode,
                                  ws_uint32_t masking_key,
                                  const char *payload,
                                  size_t payload_len,
                                  size_t *size)
{
  ws_uint8_t payload_len_high;
  ws_uint8_t payload_ext_len_size;
  ws_uint16_t header;
  ws_uint8_t *data;
  ws_uint16_t offset = 0;
  size_t frame_size;

  (void)masking_key; /* TODO: Implement masking */

  if (payload_len < PAYLOAD_LENGTH_16) {
    payload_len_high = (ws_uint8_t)payload_len;
    payload_ext_len_size = 0;
  } else if (payload_len <= (ws_uint64_t)0xFFFF) {
    payload_len_high = PAYLOAD_LENGTH_16;
    payload_ext_len_size = (ws_uint8_t)sizeof(ws_uint16_t);
  } else {
    payload_len_high = PAYLOAD_LENGTH_64;
    payload_ext_len_size = (ws_uint8_t)sizeof(ws_uint64_t);
  }

  frame_size = sizeof(header)
    + payload_ext_len_size
    + ((flags & WS_FLAG_MASK) != 0 ? sizeof(masking_key) : 0)
    + payload_len;
  data = malloc(frame_size);
  if (data == NULL) {
    return NULL;
  }

  *size = frame_size;

  header = htons(
    (flags & 0xF080) | ((opcode & 0x0F) << 8) | (payload_len_high & 0x7F));
  memcpy(data, &header, sizeof(header));
  offset += sizeof(header);

  if (payload_ext_len_size != 0) {
    switch (payload_ext_len_size) {
      case sizeof(ws_uint16_t):
        *(ws_uint16_t *)(data + offset) = htons((ws_uint16_t)payload_len);
        break;
      case sizeof(ws_uint64_t):
        *(ws_uint64_t *)(data + offset) = htonll(payload_len);
        break;
    }
    offset += payload_ext_len_size;
  }

  if ((flags & WS_FLAG_MASK) != 0) {
    *(ws_uint16_t *)(data + offset) = htons(masking_key);
    offset += sizeof(ws_uint16_t);
  }

  memcpy(data + offset, payload, payload_len);

  return data;
}

int ws_send(ws_socket_t sock,
            int opcode,
            const char *data,
            size_t len,
            ws_uint32_t flags,
            ws_uint32_t masking_key)
{
  ws_uint8_t *frame;
  size_t frame_size;
  int error;

  frame = ws_alloc_frame(
    flags, opcode, masking_key, data, len, &frame_size);
  if (frame == NULL) {
    return -1;
  }

  error = ws_socket_send(sock, (const char *)frame, (int)frame_size);
  free(frame);

  return error;
}

int ws_send_text(ws_socket_t sock,
                 const char *text,
                 ws_uint16_t flags,
                 ws_uint32_t masking_key)
{
  return ws_send(sock, WS_OP_TEXT, text, strlen(text), flags, masking_key);
}

int ws_send_close(ws_socket_t sock, ws_uint16_t flags, ws_uint32_t masking_key)
{
  return ws_send(sock, WS_OP_CLOSE, NULL, 0, 0, 0);
}

int ws_recv(
    ws_socket_t sock,
    int *popcode,
    int *pflags,
    void **data,
    size_t *len)
{
  int error;
  ws_uint16_t header;
  int opcode;
  int mask;
  char *payload;
  size_t payload_len = 0;
  char masking_key[4];
  size_t i;

  assert(popcode != NULL);
  assert(data == NULL || len != NULL);

  error = ws_socket_recv(sock, (void *)&header, sizeof(header), NULL);
  if (error <= 0) {
    return error;
  }

  header = ntohs(header);
  opcode = (header & 0x0F00) >> 8;
  mask = (header & WS_FLAG_MASK) != 0;
  payload_len = header & 0x7F; 

  *popcode = opcode;

  if (pflags != NULL) {
    *pflags = header & 0xFF00;
  }

  if (payload_len == PAYLOAD_LENGTH_16) {
    ws_uint16_t len;
    error = ws_socket_recv(sock, (void *)&len, sizeof(len), NULL);
    if (error <= 0) {
      return error;
    }
    payload_len = ntohs(len);
  } else if (payload_len == PAYLOAD_LENGTH_64) {
    ws_uint64_t len;
    error = ws_socket_recv(sock, (void *)&len, sizeof(len), NULL);
    if (error <= 0) {
      return error;
    }
    payload_len = ntohll(len) & 0x7FFFFFFF; /* Clear MSB */
  }

  if (mask) {
    error = ws_socket_recv(sock, masking_key, sizeof(masking_key), NULL);
    if (error <= 0) {
      return error;
    }
  }

  if (data != NULL) {
    if (payload_len > 0) {
      payload = malloc(payload_len);
      if (payload == NULL) {
        return -1;
      }
      error = ws_socket_recv(sock, payload, (int)payload_len, NULL);
      if (error <= 0) {
        free(payload);
        return error;
      }
      if (mask) {
        for (i = 0; i < payload_len; i++) {
          payload[i] ^= masking_key[i % 4];
        }
      }
      *data = payload;
    }
    *len = payload_len;
  } else {
    char buf[1];
    for (i = 0; i < payload_len; i++) {
      error = recv(sock, buf, 1, 0);
      if (error <= 0) {
        return error;
      }
    }
    if (len != NULL) {
      *len = payload_len;
    }
  }

  return 0;
}
