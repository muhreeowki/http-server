#include "transport.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  struct transport *server;

  if (argc != 2) {
    fprintf(stderr, "usage: %s hostname\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if ((server = new_http_server(argv[1])) == NULL) {
    exit(1);
  }
  if (start_server(server) == -1) {
    exit(EXIT_FAILURE);
  }
  close_server(server);
  return EXIT_SUCCESS;
}

// TEST the HTTP Parser
// int main(int argc, char *argv[]) {
//   struct http_request *req;
//   char *data;
//
//   if (argc != 2) {
//     fprintf(stderr, "usage: %s string", argv[0]);
//     exit(1);
//   }
//
//   req = malloc(sizeof(struct http_request));
//   parse_http_request(argv[1], req);
//
//   print_http_request(req);
//   free(req);
//   return EXIT_SUCCESS;
// }
