#ifndef WS_SOCKET_H
#define WS_SOCKET_H

#include <stdlib.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef int ws_socklen_t;
  typedef SOCKET ws_socket_t;
  #define SHUT_RD SD_RECEIVE
  #define SHUT_WR SD_SEND
  #define SHUT_RDWR SD_BOTH
  #define ws_close_socket closesocket
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <unistd.h>
  typedef int ws_socket_t;
  #define ws_close_socket close
#endif

typedef int (*recv_handler_t)(
    const char *buf, int len, int chunk_offset, int chunk_len);

int ws_socket_recv(ws_socket_t sock, char *buf, int size, recv_handler_t handler);
int ws_socket_send(ws_socket_t sock, const char *buf, int size);
int ws_socket_send_string(ws_socket_t sock, char *s);

int ws_socket_last_error(void);

#endif /* WS_SOCKET_H */
