#include <arpa/inet.h>
#include <asm-generic/socket.h>
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
#define MAXDATALEN 100

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  int status, sock_fd, numbytes;
  struct addrinfo hints;
  struct addrinfo *servinfo, *p;
  struct sigaction sa;

  socklen_t sin_size;
  struct sockaddr_storage conn_addr;
  char addr_str[INET6_ADDRSTRLEN];

  if (argc != 2) {
    fprintf(stderr, "usage: %s hostname\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(argv[1], PORT, &hints, &servinfo) != 0)) {
    fprintf(stderr, "gai error: %s\n", gai_strerror(status));
    return 1;
  }

  // Loop through the gai results and connect to the first one you can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    // get a socket with socket()
    if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
        -1) {
      perror("client: socket");
      continue;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&p->ai_addr),
              addr_str, sizeof(addr_str));
    printf("client: attempting connection to %s\n", addr_str);

    if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client error: failed to connect to server\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&p->ai_addr), addr_str,
            sizeof(addr_str));
  printf("client: attempting connection to %s\n", addr_str);

  freeaddrinfo(servinfo);

  // recieve messages using recv()
  char buf[MAXDATALEN];
  if ((numbytes = recv(sock_fd, &buf, MAXDATALEN - 1, 0)) == -1) {
    perror("client: recv");
    exit(1);
  }

  buf[numbytes] = '\0';

  printf("client recieved: %s\n", buf);

  close(sock_fd);

  return 0;
}
