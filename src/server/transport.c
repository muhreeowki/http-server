#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#include "transport.h"

void *get_in_addr(struct sockaddr *sa);
void sigchld_handler(int s);
int socket_bind(struct addrinfo *p);
int accept_loop(struct transport *t);
struct transport *new_transport(char *host, char *service,
                                struct addrinfo *hints, conn_handler *handler);

struct transport *new_http_server(char *port) {
  struct addrinfo hints;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // TODO: USE HTTP HANDLER
  return new_transport(NULL, port, &hints, &http_handler);
}

struct transport *new_transport(char *host, char *port, struct addrinfo *hints,
                                conn_handler *handler) {
  int gai_status;
  struct addrinfo *servinfo;
  struct transport *t;

  if ((t = malloc(sizeof(struct transport))) == NULL) {
    perror("new_transport");
    return NULL;
  }

  t->handler = handler;
  t->port = port;

  if ((gai_status = getaddrinfo(host, port, hints, &servinfo) != 0)) {
    fprintf(stderr, "gai error: %s\n", gai_strerror(gai_status));
    return NULL;
  }

  if ((t->sock_fd = socket_bind(servinfo)) == -1) {
    fprintf(stderr, "server error: failed to bind\n");
    freeaddrinfo(servinfo);
    return NULL;
  }

  freeaddrinfo(servinfo);

  return t;
}

int start_server(struct transport *t) {
  // listen on the socket using listen()
  if (listen(t->sock_fd, 10) == -1) {
    perror("server: listen");
    return -1;
  }

  printf("server listening for connections on port %s...\n", t->port);

  // accept connections using accept()
  if (accept_loop(t) == -1) {
    perror("server: accept_loop");
    return -1;
  }

  return 0;
}

void close_server(struct transport *t) {
  close(t->sock_fd);
  free(t);
}

int socket_bind(struct addrinfo *p) {
  int sock_fd = -1, yes = 1;
  // Loop through the gai results and connect to the first one you can
  for (; p != NULL; p = p->ai_next) {
    // get a socket with socket()
    if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("server: socket");
      continue;
    }
    // set socket options
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
      perror("setsocketopt");
      exit(EXIT_FAILURE);
    }
    // bind the socket to a port using bind()
    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      perror("server: bind");
      continue;
    }
    break;
  }

  return sock_fd;
}

int accept_loop(struct transport *t) {
  int conn_sock_fd;
  socklen_t sin_size;
  char conn_str[INET6_ADDRSTRLEN];

  struct sockaddr_storage conn_addr;
  struct sigaction sa;

  // Handle sigaction
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  for (;;) {
    sin_size = sizeof(conn_addr);

    if ((conn_sock_fd = accept(t->sock_fd, (struct sockaddr *)&conn_addr,
                               &sin_size)) == -1) {
      perror("server: accept");
      continue;
    }

    inet_ntop(conn_addr.ss_family, get_in_addr((struct sockaddr *)&conn_addr),
              conn_str, sizeof(conn_str));
    printf("server: accepted connection from %s\n", conn_str);

    // Create child process to handle connection
    switch (fork()) {
    case 0: // child
      if (t->handler(conn_sock_fd, conn_str) == -1)
        perror("transport_handler");
      break;
    case -1: // error
      return -1;
    default: // partent
      close(conn_sock_fd);
    }
  }
  return 0;
}

void sigchld_handler(int s) {
  (void)s; // quiet unused variable warning

  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
