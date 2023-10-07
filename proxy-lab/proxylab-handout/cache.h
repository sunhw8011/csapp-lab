#ifndef __CACHE_H__
#define __CACHE_H__

#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAX_CACHELINE_NUM 10

typedef struct
{
    char uri[MAXLINE];
    char obj[MAX_OBJECT_SIZE];
} CacheLine;

typedef struct
{
    CacheLine data[MAX_CACHELINE_NUM];
    int current_ptr;    // 用来记录当前应该被替换调的CacheLine
    
    int reader_count;
    sem_t mutex_reader;
    sem_t mutex_write; 
} Cache;

Cache* init_cache();
int reader(Cache *cache, int fd, char *uri);
void writer(Cache *cache, char *uri, char *buf);
void delete_cache(Cache *cache);

#endif