#include "transport.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>

// Basic Test handler
int nop_handler(int conn_sock_fd, char *conn_str) {
  char payload[] = {"hello there from my C server."};
  if (send(conn_sock_fd, payload, strlen(payload), 0) == -1)
    perror("server: send");
  close(conn_sock_fd);

  printf("server: handled and closed connection from %s\n", conn_str);
  exit(0);
}

// HTTP HANDLER
struct http_request *http_strtoreq(char *req_str, struct http_request *req) {
  int len = strlen(req_str), line_count = 0;
  char *temp, *http_data = req_str, *http_body = NULL, *first_line = NULL,
              *delim = "\n\n"; // The empty line between the data and the body

  req->req_str = strdup(req_str);
  // get the body first by spliting data into two parts, the request data and
  // the request body
  for (int i = 0; i < len; i++) {
    if (*(req_str + i) == '\n') {
      line_count++;
      if (i + 1 < len && *(req_str + i + 1) == '\n') {
        *(req_str + ++i) = '\0';
        if (i + 1 < len) {
          http_body = req_str + i + 1;
        }
        break;
      }
    }
  }
  req->body = http_body;

  // Get the headers from the http data.
  // First allocate memory for the headers
  if ((req->headers = malloc(sizeof(char *) * line_count + 1)) == NULL) {
    perror("malloc");
    return NULL;
  }

  // Now split up the request data;
  // Get the first line
  if ((first_line = strtok(http_data, "\r\n")) == NULL) {
    return NULL;
  }
  // NOTE: This might be a redundant check
  if (strlen(first_line) == 0) {
    return NULL; // bad request
  }

  // loop through the lines if any
  int i = 0;
  *(req->headers) = NULL;
  while (i < line_count) {
    if ((temp = strtok(NULL, "\r\n")) == NULL)
      break;
    *(req->headers + i++) = strdup(temp);
    *(req->headers + i) = NULL;
  }

  // Split up the first line into <method> <target> <protocol-version>
  // Extract METHOD
  if ((req->method = strtok(first_line, " ")) == NULL) {
    return NULL; // bad request
  }
  // Extract PATH or URL Destination
  if ((req->target = strtok(NULL, " ")) == NULL) {
    return NULL; // bad request
  }
  // Extract HTTP version
  if ((req->version = strtok(NULL, " \r\n")) == NULL) {
    return NULL; // bad request
  }

  return req;
}

void http_printreq(struct http_request *req) { printf("%s\n", req->req_str); }

struct http_response *build_http_response(char *version, int code, char *msg,
                                          char *body, char **headers) {
  struct http_response *resp;

  if ((resp = malloc(sizeof(struct http_response))) == NULL) {
    perror("build_http_response");
    return NULL;
  }

  resp->status_code = code;
  resp->status_msg = msg;
  resp->version = version;
  resp->body = body;
  resp->headers = headers;

  return resp;
}

char *http_resptostr(struct http_response *resp) {
  if (!resp || !resp->version || !resp->status_msg) {
    printf("missing response struct.\n");
    return NULL;
  }

  // Status line size
  size_t size = strlen(resp->version) + strlen(resp->status_msg) +
                7; // 7 = 2 spaces, 3 integers, 1 \n and 1 \r
  size_t body_len = resp->body ? strlen(resp->body) : 0;

  // Headers size
  if (resp->headers) {
    for (int i = 0; resp->headers[i] != NULL; i++) {
      size += strlen(resp->headers[i]) + 2; // +2 for \r\n
    }
  }

  // Body size
  size += body_len;
  size += 3; // 2 for blank line, 1 for null

  char *buf = malloc(size);
  if (!buf) {
    perror("resptostr");
    return NULL;
  }

  // Write first line
  int written = 0;
  written += snprintf(buf, size, "%s %d %s\r\n", resp->version,
                      resp->status_code, resp->status_msg);

  // Write the Headers
  if (resp->headers) {
    for (int i = 0; resp->headers[i] != NULL; i++)
      written += snprintf(buf + written, size, "%s\r\n", resp->headers[i]);
  }

  // Write blank line
  written += snprintf(buf + written, size, "\r\n");

  // Write body
  if (resp->body) {
    memcpy(buf + written, resp->body, body_len);
  }
  buf[written + body_len] = '\0';

  return buf;
}

int recv_http_request(int conn_sock_fd, struct http_request *req) {
  char buf[MAX_MSG_SIZE];
  int len, writen_len;
  struct http_response *resp;

  // 2. read the request message from the connection using recv()
  if ((len = recv(conn_sock_fd, &buf, MAX_MSG_SIZE - 1, 0)) == -1)
    return -1;
  buf[len] = '\0';

  // 3. parse the message into a http request
  if ((http_strtoreq(buf, req)) == NULL) {
    fprintf(stderr, "bad request");
    return -1;
  }

  return len;
}

int send_http_response(int conn_sock_fd, struct http_response *resp) {
  char *resp_str = NULL;

  // Convert to string
  if ((resp_str = http_resptostr(resp)) == NULL) {
    printf("error converting respons to string.\n");
    return -1;
  }

  // Send the data
  printf("writing response:\n%s", resp_str);
  return send(conn_sock_fd, resp_str, strlen(resp_str) + 1, 0);
}

int http_handler(int conn_sock_fd, char *conn_str) {
  int writen_len;
  struct http_response *resp;
  struct http_request req;

  // read and parse http request
  if ((recv_http_request(conn_sock_fd, &req)) == -1) {
    fprintf(stderr, "recv_http_request: failed to read http request.\n");
    return -1;
  }

  // log http request
  http_printreq(&req);

  // TODO:
  // parse the http request into a router function
  //    - router function will pass the request to the specific path/url/target
  //    handler.
  //    - target handler will write a http response

  char *headers[] = {
      "Content-Type: text/html",
      NULL // sentinel
  };

  resp = build_http_response(
      strdup(req.version), 200, "ok",
      "<html><head><title>Hello World</title></head><body><h1>Hello "
      "World!<h1></body></html>",
      headers);

  if ((writen_len = send_http_response(conn_sock_fd, resp)) == -1) {
    printf("failed to write response.");
    return -1;
  }

  return 0;
}
