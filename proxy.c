#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *host_hdr = "Host: %s\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

static const char *user_agent_key = "User_Agent";
static const char *host_key = "Host";
static const char *connection_key = "Connection";
static const char *proxy_connection_key = "Proxy_Connection";


void doit(int fd);
void make_requesthdrs(char *header, char *hostname, char *path, rio_t *rp);
int parse_uri(char *url, char *hostname, char *port, char *filename);
void *thread(void *vargp);

void doit(int fd)
{
  int clientfd; // 클라이언트 파일 디스크립터
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], server_header[MAXLINE];
  char hostname[MAXLINE], path[MAXLINE], portnumber[MAXLINE];
  rio_t rio, server_rio;
  size_t n;

  Rio_readinitb(&rio, fd); // 버퍼 rio와 파일 식별자 fd를 연결
  Rio_readlineb(&rio, buf, MAXLINE); // 버퍼 rio에서 읽은 내용을 buf에 MAXLINE 만큼 복사
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  printf("%s--%s--%s", method, uri, version);
  parse_uri(uri, hostname, portnumber, path);

  make_requesthdrs(server_header, hostname, path, &rio);

  clientfd = Open_clientfd(hostname, portnumber);
  Rio_readinitb(&server_rio, clientfd);
  Rio_writen(clientfd, server_header,strlen(server_header));

  while((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0)
  {
      printf("%s\n", buf);
      Rio_writen(fd, buf, n);
  }
  Close(clientfd);
}

void make_requesthdrs(char *header, char *hostname, char *path, rio_t *rp)
{
  char buf[MAXLINE], request_header[MAXLINE], host_header[MAXLINE], other_header[MAXLINE];

  sprintf(request_header, "GET %s HTTP/1.0\r\n", path);
  while(Rio_readlineb(rp, buf, MAXLINE) > 0)
  {
    if(strcmp(buf, "\r\n") == 0) break;

    if (!strncasecmp(buf, host_key, strlen(host_key)))
    {
      strcpy(host_header, buf);
      continue;
    }
    
    if(strncasecmp(buf, connection_key, strlen(connection_key)) && strncasecmp(buf, proxy_connection_key, strlen(proxy_connection_key)) && strncasecmp(buf, user_agent_key, strlen(user_agent_key)))
    {
      strcat(other_header, buf);
    }
  }

  if(strlen(host_header) == 0)
  {
    sprintf(host_header, host_hdr, hostname);
  }
  
  sprintf(header, "%s%s%s%s%s%s%s", request_header, host_header, connection_hdr, proxy_connection_hdr, user_agent_hdr, other_header, "\r\n");
  return;
}

// 들어온 uri를 기본 주소, 포트 번호, 추가 경로로 파싱
int parse_uri(char *url, char *hostname, char *port, char *filename)
{
  char *p;
  char arg1[MAXLINE], arg2[MAXLINE];
  
  if(p = strchr(url, '/'))
  {
    sscanf(p + 2, "%s", arg1);
  }
  else
  {
    strcpy(arg1, url);
  }

  if (p = strchr(arg1, ':')){     // 포트 번호가 함께 들어온 경우 hostname:port/home.html
    *p = '\0';
    sscanf(arg1, "%s", hostname);       // hostname
    sscanf(p+1, "%s", arg2);            // port/home.html?n1=5&n2=10

    p = strchr(arg2, '/');
    *p = '\0';
    sscanf(arg2, "%s", port);           // port
    *p = '/';
    sscanf(p, "%s", filename);        // home.html?n1=5&n2=10
    
    printf("%s이랑 %s이고 %s임\n", hostname, port, filename);
  }
  else{                           // 포트 번호가 포함되지 않은 경우 hostname.1/home.html
    p = strchr(arg1, '/');
    *p = '\0';
    sscanf(arg1, "%s", hostname);
    *p = '/';
    sscanf(p, "%s", filename);
    // printf("%s %s\n", hostname, filename);
  }
  return;
}

void *thread(void *vargp)
{
  int connfd = (int)vargp;
  Pthread_detach(pthread_self());
  doit(connfd);
  Close(connfd);
}

int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  pthread_t tid;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);
  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);
    Pthread_create(&tid, NULL, thread, (void *)connfd);
    // doit(connfd);
    // Close(connfd);
  }
  printf("%s", user_agent_hdr);
  return 0;
}
