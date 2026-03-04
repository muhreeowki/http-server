#include "transport.h"
#include <stdio.h>
#include <stdlib.h>

int handleHome(struct httpRequest *req, struct httpResponse *resp) {
  char *headers[] = {"Content-Type: text/html", NULL};

  printf("Handling home route.\n");

  resp->headers = headers;
  resp->body = "<html><head><title>Hello World</title></head><body><h1>Hello"
               "World</h1><p>this is a test bro, please work</p></body></html>";
  resp->version = req->version;
  resp->status_code = 200;
  resp->status_msg = "ok";

  return 0;
}

int main(int argc, char *argv[]) {
  struct httpServer *server;

  if (argc != 2) {
    fprintf(stderr, "usage: %s hostname\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  struct httpRouter router;
  initializeHttpRouter(&router);

  if (newRouteHandler(&router, "/home", &handleHome) == -1) {
    fprintf(stderr, "failed to create new route handler.\n");
    exit(1);
  }

  if ((server = newHTTPServer(argv[1], &router)) == NULL) {
    fprintf(stderr, "failed to create new http server.\n");
    exit(1);
  }

  if (startHTTPServer(server) == -1) {
    fprintf(stderr, "failed to start http server.\n");
    exit(EXIT_FAILURE);
  }

  closeHTTPServer(server);
  return EXIT_SUCCESS;
}
