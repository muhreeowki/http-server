#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <netdb.h>

#define MAX_MSG_SIZE 1024

typedef int conn_handler(int conn_sock_fd, char *conn_str);

struct transport {
  struct addrinfo server_info;
  int sock_fd;
  char *port;
  conn_handler *handler; // Function to handle connections; This is where you
                         // can setup different connection handlers that can use
                         // different application layer protocals.
};

struct http_request {
  char *method;
  char *target;
  char *version;
  char **headers;
  char *body;
};

// SERVER FUNCTIONS
struct transport *new_http_server(char *port);
int start_server(struct transport *t);
void close_server(struct transport *t);
struct http_request *parse_http_request(char *raw, struct http_request *dest);
void print_http_request(struct http_request *req);

// HANDLER FUNCTIONS
int nop_handler(int conn_sock_fd, char *conn_str);

#endif // !TRANSPORT_H
