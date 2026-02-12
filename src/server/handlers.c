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

int nop_handler(int conn_sock_fd, char *conn_str) {
  char payload[] = {"hello there from my C server."};
  if (send(conn_sock_fd, payload, strlen(payload), 0) == -1)
    perror("server: send");
  close(conn_sock_fd);

  printf("server: handled and closed connection from %s\n", conn_str);
  exit(0);
}

struct http_response *build_http_response(char *version, int code, char *msg,
                                          char *body, char **headers);

int http_handler(int conn_sock_fd, char *conn_str) {
  char msg[MAX_MSG_SIZE];
  int len, writen_len;
  struct http_response *resp;
  struct http_request req;

  // TODO:
  // 1. handle a handshake with the client using send()

  // 2. read the request message from the connection using recv()
  if ((len = recv(conn_sock_fd, &msg, MAX_MSG_SIZE - 1, 0)) == -1)
    return -1;
  msg[len] = '\0';

  // 3. parse the message into a http request
  if ((http_strtoreq(msg, &req)) == NULL) {
    fprintf(stderr, "bad request");
    return -1;
  }
  print_http_request(&req);

  // TODO:
  // 4. parse the http request into a router function
  //    - router function will pass the request to the specific path/url/target
  //    handler.

  // TODO:
  // 5. write a http response

  char *headers[] = {
      "Content-Type: text/html",
      NULL // sentinel
  };

  resp = build_http_response(
      strdup(req.version), 200, "ok",
      "<html><head><title>Hello World</title></head><body><h1>Hello "
      "World!<h1></body></html>",
      headers);

  if ((writen_len = write_response(conn_sock_fd, resp)) == -1) {
    printf("failed to write response.");
    return -1;
  }

  return 0;
}

struct http_request *http_strtoreq(char *raw, struct http_request *req) {
  int len = strlen(raw), line_count = 0;
  char *temp, *http_data = raw, *http_body = NULL, *first_line = NULL,
              *delim = "\n\n"; // The empty line between the data and the body

  // get the body first by spliting data into two parts, the request data and
  // the request body
  for (int i = 0; i < len; i++) {
    if (*(raw + i) == '\n') {
      line_count++;
      if (i + 1 < len && *(raw + i + 1) == '\n') {
        *(raw + ++i) = '\0';
        if (i + 1 < len) {
          http_body = raw + i + 1;
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

void print_http_request(struct http_request *req) {
  printf("%s %s %s\n", req->method, req->target, req->version);
  for (int i = 0; *(req->headers + i) != NULL; i++) {
    printf("%s\n", *(req->headers + i));
  }
  printf("%s\n", req->body == NULL ? "" : req->body);
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

  int written = 0;
  // Write first line
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

int write_response(int conn_sock_fd, struct http_response *resp) {
  char *resp_str = NULL;

  if ((resp_str = http_resptostr(resp)) == NULL) {
    printf("error converting respons to string.\n");
    return -1;
  }

  printf("writing response:\n%s", resp_str);
  return send(conn_sock_fd, resp_str, strlen(resp_str) + 1, 0);
}
