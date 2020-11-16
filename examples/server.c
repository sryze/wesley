/* Simple WebSocket echo server */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#ifdef _WIN32
  #include <winsock2.h>
  #define poll WSAPoll
  typedef WSAPOLLFD pollfd_t;
#else
  #include <poll.h>
  typedef struct pollfd pollfd_t;
#endif
#include "ws.h"
#include "ws_http.h"
#include "ws_socket.h"
#include "ws_string.h"

#define PORT 8080
#define MAX_CLIENTS 32

struct ws_client {
  int id;
  ws_socket_t socket;
  struct sockaddr_in address;
};

static struct ws_client clients[MAX_CLIENTS];

static bool handle_client(ws_socket_t sock, struct sockaddr_in *addr)
{
  int i;
  int error;
  char ip_str[INET_ADDRSTRLEN] = {0};
  struct ws_client *client = NULL;

  error = ws_accept(sock);
  if (error != 0) {
    printf("Handshake error: %s\n", ws_error_message(error));
    return false;
  }

  for (i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].socket == -1) {
      client = &clients[i];
      client->socket = sock;
      client->address = *addr;
      break;
    }
  }
  if (client == NULL) {
    printf("Client limit reached, closing connection\n");
    ws_send_close(sock, 0, 0);
    return false;
  }

  if (inet_ntop(addr->sin_family,
                &addr->sin_addr,
                ip_str,
                sizeof(ip_str)) != NULL) {
    printf("Client %d has connected (%s)\n", i, ip_str);
  } else {
    printf("Client %d has connected (unknown address)\n", i);
  }

  return true;
}

static void handle_message(struct ws_client *client)
{
  int result;
  int opcode;
  int flags;
  void *data;
  size_t length;

  result = ws_recv(client->socket, &opcode, &flags, &data, &length);
  if (result != 0) {
    printf("Error receiving message from client: %d\n",
            ws_socket_last_error());
    return;
  }

  if (opcode < WS_OP_CLOSE) {
    result = ws_send(client->socket,
            opcode,
            data,
            length,
            flags & WS_FLAG_FINAL,
            0);
    if (result < 0) {
      printf("Error sending message to client: %d\n",
              ws_socket_last_error());
    }
  } else if (opcode == WS_OP_CLOSE) {
    printf("Client %d has disconnected\n", client->id);
    ws_close_socket(client->socket);
    client->socket = -1;
  } else {
    printf("Opcode is not implemented: %d\n", opcode);
  }

  free(data);
}

int main(int argc, char **argv)
{
  const char *port_str;
  int port;
  ws_socket_t server_sock;
  ws_socket_t client_sock;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  pollfd_t pollfds[MAX_CLIENTS + 1];
  int opt;

  port_str = getenv("PORT");
  port = port_str != NULL ? atoi(port_str) : PORT;

#ifdef _WIN32
  WSADATA wsa_data;
  DWORD result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (result != 0) {
    printf("WSAStartup failed: %d\n", result);
    return 1;
  }
#endif

  for (int i = 0; i < MAX_CLIENTS; i++) {
    clients[i].id = i;
    clients[i].socket = -1;
  }

  server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_sock < 0) {
    printf("Could not open socket: %d\n", ws_socket_last_error());
    return 1;
  }

  opt = 1;
  setsockopt(server_sock,
    SOL_SOCKET,
    SO_REUSEADDR,
    (const void *)&opt,
    sizeof(opt));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);
  if (bind(server_sock,
    (struct sockaddr *)&server_addr,
    sizeof(server_addr)) != 0) {
    ws_close_socket(server_sock);
    printf("Could not bind to port %u: %d\n", port, ws_socket_last_error());
    return 1;
  }

  if (listen(server_sock, 1) != 0) {
    ws_close_socket(server_sock);
    printf("Could not start listening for connections: %d\n",
      ws_socket_last_error());
    return 1;
  }

  printf("Listening on port %u\n", port);

  for (;;) {
    int i;
    int result;

    for (i = 0; i < MAX_CLIENTS; i++) {
      pollfds[i].fd = clients[i].socket;
      pollfds[i].events = clients[i].socket != -1 ? POLLIN : 0;
      pollfds[i].revents = 0;
    }

    /* Server's socket */
    pollfds[MAX_CLIENTS].fd = server_sock;
    pollfds[MAX_CLIENTS].events = POLLIN;

    result = poll(pollfds, MAX_CLIENTS + 1, 10);
    if (result < 0) {
      printf("Poll failed: %d\n", ws_socket_last_error());
      break;
    }

    if (result > 0) {
      if ((pollfds[MAX_CLIENTS].revents & POLLIN) != 0) {
        /* Listening socket is ready to accept a connection */
        client_sock = accept(server_sock,
          (struct sockaddr *)&client_addr,
          &client_addr_len);
        if (client_sock < 0) {
          printf("Could not accept connection: %d\n", ws_socket_last_error());
          continue;
        }
        if (!handle_client(client_sock, &client_addr)) {
          ws_close_socket(client_sock);
        }
      }
      for (i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != -1) {
          if ((pollfds[i].revents & POLLHUP) != 0
               || (pollfds[i].revents & POLLERR) != 0) {
            /* Client disconnected */
            printf("Lost connection with client %d\n", i);
            ws_close_socket(clients[i].socket);
            clients[i].socket = -1;
            continue;
          }

          if ((pollfds[i].revents & POLLIN) != 0) {
            /* One of the client sockets is ready for reading */
            handle_message(&clients[i]);
          }
        }
      }
    }
  }

#ifdef _WIN32
  WSACleanup();
#endif

  return 0;
}
