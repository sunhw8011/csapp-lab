#include <stdio.h>

#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

// /* Recommended max cache and object sizes */
// #define MAX_CACHE_SIZE 1049000
// #define MAX_OBJECT_SIZE 102400

// 设置默认的线程数和同时的连接数
#define NTHREADS 4
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

void doit(int fd);
void parse_uri(char *uri, char *hostname, char *port, char *path);
void build_reqheader(rio_t *rp, char *newreq, char *method, char *hostname, char *port, char *path);

void *thread(void *vargp);

sbuf_t sbuf;
Cache *cache;

int main(int argc, char** argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    char hostname[MAXLINE], port[MAXLINE];
    struct sockaddr_storage clientaddr;

    pthread_t tid;
    
    if (argc != 2) {
        fprintf(stderr, "usage :%s <port> \n", argv[0]);
        exit(1);
    }
    signal(SIGPIPE, SIG_IGN);   // 防止收到SIGPIPE信号而过早终止

    sbuf_init(&sbuf, SBUFSIZE);    // 初始化并发缓冲区
    cache = init_cache();          // 初始化缓存

    // 创建一定数量的工作线程
    for (int i=0; i<NTHREADS; ++i) {
        Pthread_create(&tid, NULL, thread, NULL);
    }

    listenfd = Open_listenfd(argv[1]);
    while(1) {
        clientlen = sizeof(clientaddr);
	    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        // 向信号量缓冲区中写入这个文件描述符
        sbuf_insert(&sbuf, connfd);
        // doit(connfd);
        // Close(connfd);
    }
    
    // printf("%s", user_agent_hdr);
    return 0;
}

void doit(int fd) {
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], object_buf[MAX_OBJECT_SIZE];
    int serverfd;
    int n;
    int total_size;
    rio_t client_rio, server_rio;

    char hostname[MAXLINE], port[MAXLINE], path[MAXLINE];
    char newreq[MAXLINE];
    char complete_uri[MAXLINE];

    // 面对客户端，自己是服务器，接收客户端发送的请求
    Rio_readinitb(&client_rio, fd);
    if (!Rio_readlineb(&client_rio, buf, MAXLINE)) {
        return;
    }
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("client uri is: %s\n", uri);

    parse_uri(uri, hostname, port, path);   // 解析uri中元素，注意这个函数会改变uri

    // 尝试去读缓存内容
    sprintf(complete_uri, "%s%s", complete_uri, hostname);
    sprintf(complete_uri, "%s%s", complete_uri, port);
    sprintf(complete_uri, "%s%s", complete_uri, path);
    // 如果直接命中缓存，那就没有必要继续了
    if (reader(cache, fd, complete_uri)) {
        fprintf(stdout, "%s from cache\n", uri);
        fflush(stdout);
        return;
    }

    build_reqheader(&client_rio, newreq, method, hostname, port, path); //构造新的请求头
    //pintf("the new req is: \n");
    //printf("%s", newreq);

    // 与目标服务器建立连接
    serverfd = Open_clientfd(hostname, port);
    if (serverfd < 0) {
        fprintf(stderr, "connect to real server err");
        return;
    }

    // 发送请求报文
    Rio_readinitb(&server_rio, serverfd);
    Rio_writen(serverfd, newreq, strlen(newreq));

    // 将目标服务器的内容接收后原封不动转发给客户端
    total_size = 0;
    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE))!=0) {
        printf("get %d bytes from server\n", n);
        Rio_writen(fd, buf, n);
        strcpy(object_buf + total_size, buf);
        total_size += n;
    }
    if (total_size < MAX_OBJECT_SIZE) {
        writer(cache, complete_uri, object_buf);
    }

    Close(serverfd);    // 别忘了关闭文件描述符
}

void parse_uri(char *uri, char *hostname, char *port, char *path) {
    // 假设uri: http://localhost:12345/home.html
    char *hostpos = strstr(uri, "//");
    if (hostpos==NULL) {
        // 此种情况下，uri为 /home.html
        char *pathpos = strstr(uri, "/");
        if (pathpos != NULL) {
            strcpy(path, pathpos);
            strcpy(port, "80"); //没有包含端口信息，则默认端口为80
            return;
        }
        hostpos = uri;
    } else {
        hostpos += 2;
    }
    char *portpos = strstr(hostpos, ":");
    // hostpos为：//localhost:12345/home.html
    // portpos为：:12345/home.html
    if (portpos != NULL) {
        int portnum;
        char *pathpos = strstr(portpos, "/");
        if (pathpos!=NULL) {
            sscanf(portpos+1, "%d%s", &portnum, path);
        } else {
            sscanf(portpos+1, "%d", &portnum);
        }
        sprintf(port, "%d", portnum);
        //printf("此时uri为:%s\n", uri);
        *portpos = '\0';    // uri被改就是因为这一步！
        //printf("这时uri为:%s\n", uri);
    } else {
        char *pathpos = strstr(hostpos, "/");
        if (pathpos != NULL) {
            strcpy(path, pathpos);
            *pathpos = '\0';
        }
        strcpy(port, "80");
    }
    strcpy(hostname, hostpos);
    return;
}

void build_reqheader(rio_t *rp, char *newreq, char *method, char *hostname, char *port, char *path) {
    sprintf(newreq, "%s %s HTTP/1.0\r\n", method, path);

    char buf[MAXLINE];
    // 循环从客户端输入中读取行
    while (Rio_readlineb(rp, buf, MAXLINE) > 0) {
        if (!strcmp(buf, "\r\n")) break;    // 空行，表示请求头已经结束
        if (strstr(buf, "Host:")!=NULL) continue; // Host由我们自己设置
        if (strstr(buf, "User-Agent:")!=NULL) continue; // User-Agent由我们自己设置
        if (strstr(buf, "Connection:")!=NULL) continue; // Connection由我们自己设置
        if (strstr(buf, "Proxy-Connection:")!=NULL) continue; // Proxy-Connection由我们自己设置

        sprintf(newreq, "%s%s", newreq, buf);   // 其他请求头直接原封不动加入请求中
    }
    // 添加上请求的必要信息
    sprintf(newreq, "%sHost: %s:%s\r\n", newreq, hostname, port);
    sprintf(newreq, "%s%s", newreq, user_agent_hdr);
    sprintf(newreq, "%s%s", newreq, conn_hdr);
    sprintf(newreq, "%s%s", newreq, proxy_conn_hdr);
    sprintf(newreq, "%s\r\n", newreq);
}

void *thread(void *vargp) {
    Pthread_detach(Pthread_self()); // 首先将自己分离以方便系统回收
    while (1) {
        int connfd = sbuf_remove(&sbuf);    // 当缓冲区中有描述符时，取出并服务
        doit(connfd);
        Close(connfd);
    }
}
