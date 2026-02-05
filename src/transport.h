#ifndef TRANSPORT_H
#define TRANSPORT_H

typedef int conn_handler(int conn_sock_fd, char *conn_str);

#include <netdb.h>
struct transport {
  struct addrinfo server_info;
  int sock_fd;
  conn_handler *handler; // Function to handle connections; This is where you
                         // can setup different connection handlers that can use
                         // different application layer protocals.
};

#endif // !TRANSPORT_H
