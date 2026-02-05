#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

int nop_handler(int conn_sock_fd, char *conn_str) {
  char payload[] = {"hello there from my C server."};
  if (send(conn_sock_fd, payload, strlen(payload), 0) == -1)
    perror("server: send");
  close(conn_sock_fd);

  printf("server: handled and closed connection from %s\n", conn_str);
  exit(0);
}
