#include "transport.h"
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

int http_handler(int conn_sock_fd, char *conn_str) {
  char msg[MAX_MSG_SIZE];
  int len, writen_len;
  struct http_response resp;
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

  // 4. parse the http request into a router function
  //    - router function will pass the request to the specific path/url/target
  //    handler.

  // 5. write a http response

  if ((writen_len = write_response(conn_sock_fd, &resp)) == -1) {
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
  if ((first_line = strtok(http_data, "\n")) == NULL) {
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
    if ((temp = strtok(NULL, "\n")) == NULL)
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
  if ((req->version = strtok(NULL, " ")) == NULL) {
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
  char *resp_str = NULL, *first_line = NULL, *headers_str = NULL, *temp = NULL,
       buf[10];
  int headers_len = 0, body_len = 0, first_line_len = 0, resp_str_len;

  if (resp->version == NULL || resp->status_msg == NULL) {
    fprintf(stderr, "bad response object.\n");
    return NULL;
  }

  // Count the leanth of all the data
  first_line_len =
      sizeof(int) + strlen(resp->status_msg) + strlen(resp->version);
  body_len = strlen(resp->body);
  if ((resp->headers) != NULL) {
    for (int i = 0; *((resp->headers) + i) != NULL; i++)
      headers_len += strlen(*((resp->headers) + i));
  }
  resp_str_len = first_line_len + body_len + headers_len + 2;

  // Allocate memory
  if ((headers_str = malloc(sizeof(char) * headers_len)) == NULL) {
    return NULL;
  }
  if ((first_line = malloc(sizeof(char) * first_line_len)) == NULL) {
    return NULL;
  }
  if ((resp_str = malloc(sizeof(char) * resp_str_len)) == NULL) {
    return NULL;
  }

  // Write the first line string
  // sprintf(first_line, "%s ", resp->version);
  // resp_str = strcat(resp_str, first_line);

  // Write the headers
  if (resp->headers != NULL) {
    for (int i = 0; *((resp->headers) + i) != NULL; i++) {
      temp = *((resp->headers) + i);
      headers_str = strcat(headers_str, temp);
    }
    resp_str = strcat(resp_str, headers_str);
  }

  // Write the body
  if (resp->body != NULL) {
    resp_str = strcat(resp_str, "\n"); // empty line
    resp_str = strcat(resp_str, resp->body);
  }

  return resp_str;
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
