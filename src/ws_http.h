#ifndef WS_HTTP_H
#define WS_HTTP_H

#include <stdbool.h>
#include "ws_socket.h"

struct ws_http_fragment {
  const char *ptr;
  size_t length;
};

const char *ws_http_parse_request_line(
  const char *buf,
  struct ws_http_fragment *method,
  struct ws_http_fragment *target,
  int *version);
const char *ws_http_parse_headers(
  const char *buf,
  void (*callback)(
    const struct ws_http_fragment *name,
    const struct ws_http_fragment *value,
    void *data),
  void *data);
int ws_http_recv_headers(ws_socket_t sock, char *headers, size_t size);
int ws_http_send_content(
  ws_socket_t sock,
  const char *content,
  size_t length,
  const char *type);
int ws_http_send_ok(ws_socket_t sock);
int ws_http_send_bad_request_error(ws_socket_t sock);
int ws_http_send_internal_error(ws_socket_t sock);

#endif /* WS_HTTP_H */
