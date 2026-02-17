#include "transport.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  struct httpServer *server;

  if (argc != 2) {
    fprintf(stderr, "usage: %s hostname\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct httpRouter router;
  initializeHttpRouter(&router);

  if ((server = newHTTPServer(argv[1], &router)) == NULL) {
    exit(1);
  }

  if (startHTTPServer(server) == -1) {
    exit(EXIT_FAILURE);
  }

  closeHTTPServer(server);
  return EXIT_SUCCESS;
}
