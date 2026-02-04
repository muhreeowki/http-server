#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

#define PORT "3000"

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

int main(void) {

  // TCP Server in C
  // Get the addrinfo struct

  int status, serv_sock_fd, conn_sock_fd, yes = 1;
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  struct sigaction sa;

  socklen_t sin_size;
  struct sockaddr_storage conn_addr;
  char conn_str[INET6_ADDRSTRLEN];

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)) {
    fprintf(stderr, "gai error: %s\n", gai_strerror(status));
    exit(EXIT_FAILURE);
  }

  // Loop through the gai results and connect to the first one you can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    // get a socket with socket()
    if ((serv_sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("server: socket");
      continue;
    }
    // set socket options
    if (setsockopt(serv_sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) ==
        -1) {
      perror("setsocketopt");
      exit(EXIT_FAILURE);
    }
    // bind the socket to a port using bind()
    if (bind(serv_sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(serv_sock_fd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    fprintf(stderr, "server error: failed to bind\n");
    exit(EXIT_FAILURE);
  }
  // listen on the socket using listen()
  if (listen(serv_sock_fd, 10) == -1) {
    perror("server: listen");
    exit(EXIT_FAILURE);
  }

  // Handle sigaction

  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  printf("server listening for connections on port %s...\n", PORT);

  // accept connections using accept()

  for (;;) {
    sin_size = sizeof(conn_addr);

    if ((conn_sock_fd = accept(serv_sock_fd, (struct sockaddr *)&conn_addr,
                               &sin_size)) == -1) {
      perror("server: accept");
      continue;
    }

    inet_ntop(conn_addr.ss_family, get_in_addr((struct sockaddr *)&conn_addr),
              conn_str, sizeof(conn_str));
    printf("server: accepted connection from %s\n", conn_str);

    // Create child process to handle connection
    if (fork() == 0) {
      close(serv_sock_fd);
      // send and recieve data from a connection using send() and recv()
      char payload[] = {"hello there from my C server."};
      if (send(conn_sock_fd, payload, strlen(payload), 0) == -1)
        perror("server: send");
      close(conn_sock_fd);
      exit(0);
    }
    close(conn_sock_fd);
  }

  return 0;
}
