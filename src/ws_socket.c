#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "ws_socket.h"

int ws_socket_recv(ws_socket_t sock,
                   char *buf,
                   int size,
                   recv_handler_t handler)
{
  int len = 0;
  int recv_len;

  for (;;) {
    if (len >= size) {
      break;
    }
    recv_len = recv(sock, buf + len, size - len, 0);
    if (recv_len <= 0) {
      return recv_len;
    }
    if (recv_len == 0) {
      break;
    }
    len += recv_len;
    if (handler != NULL && handler(buf, len, len - recv_len, recv_len)) {
      break;
    }
  }

  return len;
}

int ws_socket_send(ws_socket_t sock, const char *buf, int size)
{
  int len = 0;
  int send_len;

  for (;;) {
    if (len >= size) {
      break;
    }
    send_len = send(sock, buf + len, size - len, 0);
    if (send_len <= 0) {
      return send_len;
    }
    if (send_len == 0) {
      break;
    }
    len += send_len;
  }

  return len;
}

int ws_socket_send_string(ws_socket_t sock, char *s)
{
  size_t len = strlen(s);

  if ((size_t)len > INT_MAX) {
    return -1;
  }
  return ws_socket_send(sock, s, (int)len);
}

int ws_socket_last_error(void)
{
#ifdef _WIN32
  return WSAGetLastError();
#else
  return errno;
#endif
}
