#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <netdb.h>

#define MAX_MSG_SIZE 1024

struct httpRequest {
  char *body;
  char *method;
  char *target;
  char *version;
  char **headers;
  char *req_str;
};

struct httpResponse {
  char *body;
  char *version;
  char **headers;
  char *status_msg;
  int status_code;
};

typedef int routeHandlerFunc(struct httpRequest *req,
                             struct httpResponse *resp);

struct routeHandler {
  char *key;

  routeHandlerFunc *func;
  struct routeHandler *next;
};

struct httpRouter {
  int capacity, num_of_elements;

  struct routeHandler **arr;
};
int newRouteHandler(struct httpRouter *router, char *route,
                    routeHandlerFunc *func);
routeHandlerFunc *getRouteHandler(struct httpRouter *router, char *route);

// Function to handle connections; This is where
// you can setup different connection handlers that
// can use different application layer protocals.
typedef int protocalFunc(int conn_sock_fd, char *conn_str);

struct transport {
  struct addrinfo server_info;
  int sock_fd;
  char *port;
  protocalFunc *prtcl_func;
};

struct httpServer {
  struct transport *trnsprt;
  struct httpRouter *router;
};

// SERVER FUNCTIONS
struct httpServer *newHTTPServer(char *port, struct httpRouter *router);
int startHTTPServer(struct httpServer *s);
void closeHTTPServer(struct httpServer *s);
void initializeHttpRouter(struct httpRouter *router);

// HANDLER FUNCTIONS
int basicProtocalFunc(int conn_sock_fd, char *conn_str);
protocalFunc *httpProtcalFunc(struct httpRouter *router);

#endif // !TRANSPORT_H
