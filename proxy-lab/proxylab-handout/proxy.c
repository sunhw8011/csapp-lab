#include <stdio.h>

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void build_reqheader(rio_t *rp, char *newreq, char *hostname, char *port, char *path);

int main(int argc, char** argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    char hostname[MAXLINE], port[MAXLINE];
    struct sockaddr_storage clientaddr;
    
    if (argc != 2) {
        fprintf(stderr, "usage :%s <port> \n", argv[0]);
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);   // 防止收到SIGPIPE信号而过早终止

    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
    
    printf("%s", user_agent_hdr);
    return 0;
}

void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t client_rio, server_rio;

    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char newreq[MAXLINE];

    // 面对客户端，自己是服务器，接收客户端发送的请求
    Rio_readinitb(&client_rio, fd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) {
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("client uri is: %s\n", uri);   // 调试用
    parse_uri(uri, hostname, port, path);   // 解析uri中元素
    printf("hostname: %s\nport: %s\npath: %s\n", hostname, port, path);
    build_reqheader(&client_rio, newreq, hostname, port, path);
    printf("the new req is: \n");
    printf("%s", newreq);

}

void parse_uri(char *uri, char *hostname, char *port, char *path) {
    // 假设uri: http://localhost:12345/home.html
    char *hostpos = strstr(uri, "//");
    if (hostpos==NULL) {
        // 此种情况下，uri为 /home.html
        char *pathpos = strstr(uri, "/");
        if (pathpos != NULL) {
            strcpy(path, pathpos);
        }
        strcpy(port, "80"); //没有包含端口信息，则默认端口为80
        return;
    } else {
        char *portpos = strstr(hostpos+2, ":");
        // hostpos为：//localhost:12345/home.html
        // portpos为：:12345/home.html
        if (portpos != NULL) {
            int portnum;
            sscanf(portpos+1, "%d%s", &portnum, path);
            sprintf(port, "%d", portnum);
            *portpos = '\0';
        } else {
            char *pathpos = strstr(hostpos+2, "/");
            if (pathpos != NULL) {
                strcpy(path, pathpos);
                *pathpos = '\0';
            }
            strcpy(port, "80");
        }
        strcpy(hostname, hostpos+2);
    }
    return;
}

void build_reqheader(rio_t *rp, char *newreq, char *hostname, char *port, char *path) {
    sprintf(newreq, "GET %s HTTP/1.0\r\n", path);

    char buf[MAXLINE];
    // 循环从客户端输入中读取行
    while (Rio_readlineb(rp, buf, MAXLINE) > 0) {
        if (!strcmp(buf, "\r\n")) break;    // 空行，表示请求头已经结束
        if (strstr(buf, "Host:")!=NULL) continue; // Host由我们自己设置
        if (strstr(buf, "User-Agent:")!=NULL) continue; // User-Agent由我们自己设置

        sprintf(newreq, "%s%s", newreq, buf);   // 其他请求头直接原封不动加入请求中
    }
    // 添加上请求的必要信息
    sprintf(newreq, "%sHost: %s:%s\r\n", newreq, hostname, port);
    sprintf(newreq, "%s%s", newreq, user_agent_hdr);
    sprintf(newreq, "%s\r\n", newreq);
}
