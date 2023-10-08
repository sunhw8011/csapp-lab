#include <time.h>

#include "cache.h"

Cache* init_cache() {
    Cache *cache = (Cache*)malloc(sizeof(Cache));

    for (int i=0; i<MAX_CACHELINE_NUM; i++) {
        Sem_init(&cache->data[i].mutex_reader, 0, 1);
        Sem_init(&cache->data[i].mutex_write, 0, 1);
        cache->data[i].reader_count = 0;
        cache->data[i].timestamp = 0;
    }

    return cache;
}

int reader(Cache *cache, int fd, char *uri) {
    //printf("%s\n","进入缓存的reader函数");
    //printf("传入reader的uri为 %s\n", uri);
    int success = 0;

    // 遍历整个缓存，看看是否命中
    for (int i=0; i<MAX_CACHELINE_NUM; i++) {
        if (!strcmp(cache->data[i].uri, uri)) {
            // 增加一个读者
            printf("%s\n", "成功命中缓存");
            P(&cache->data[i].mutex_reader);
            cache->data[i].reader_count++;
            cache->data[i].timestamp = time(NULL);
            if (cache->data[i].reader_count==1) {
                P(&cache->data[i].mutex_write);
            }
            V(&cache->data[i].mutex_reader);

            // 执行读操作
            printf("%s\n", "执行读操作");
            Rio_writen(fd, cache->data[i].obj, MAX_OBJECT_SIZE);
            success = 1;

            // 减少一个读者
            printf("%s\n", "减少一个读者操作");
            P(&cache->data[i].mutex_reader);
            cache->data[i].reader_count--;
            if (cache->data[i].reader_count==0) {
                V(&cache->data[i].mutex_write);
            }
            V(&cache->data[i].mutex_reader);
            break;
        }
    }

    return success;
}

static int remove_index(Cache *cache) {
    time_t min = __LONG_MAX__;
    int result = -1;
    for (int i=0; i<MAX_CACHELINE_NUM; i++) {
        if (cache->data[i].timestamp < min) {
            min = cache->data[i].timestamp;
            result = i;
        }
    }
    return result;
}

void writer(Cache *cache, char *uri, char *buf) {
    //printf("%s\n","进入缓存的writer函数");
    //printf("传入的uri为 %s\n", uri);
    printf("传入缓存的uri为%s\n", uri);
    int i=remove_index(cache);
    P(&cache->data[i].mutex_write);
    strcpy(cache->data[i].uri, uri);
    strcpy(cache->data[i].obj, buf);
    cache->data[i].timestamp = time(NULL);
    printf("缓存中的 uri is %s\n", cache->data[i].uri);
    //printf("obj is %s\n", cache->data[i].obj);
    V(&cache->data[i].mutex_write);
}

void delete_cache(Cache *cache) {
    free(cache);
}
