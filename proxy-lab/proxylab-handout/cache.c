#include "cache.h"

Cache* init_cache() {
    Cache *cache = (Cache*)malloc(sizeof(Cache));

    Sem_init(&cache->mutex_reader, 0, 1);
    Sem_init(&cache->mutex_write, 0, 1);
    cache->current_ptr = 0;

    return cache;
}

int reader(Cache *cache, int fd, char *uri) {
    printf("%s\n","进入缓存的reader函数");
    printf("传入reader的uri为 %s\n", uri);
    int success = 0;

    // 增加一个读者计数
    P(&cache->mutex_reader);
    cache->reader_count++;
    if (cache->reader_count==1) {
        // 只要有一个读者，就占用写锁
        //printf("%s\n","写锁");
        P(&cache->mutex_write);
    }
    V(&cache->mutex_reader);

    // 遍历整个缓存，看看是否命中
    for (int i=0; i<MAX_CACHELINE_NUM; i++) {
        if (!strcmp(cache->data[i].uri, uri)) {
            Rio_writen(fd, cache->data[i].obj, MAX_OBJECT_SIZE);
            success = 1;
            printf("%s\n", "成功命中缓存");
            break;
        }
    }

    // 减少一个读者
    P(&cache->mutex_reader);
    cache->reader_count--;
    if (cache->reader_count==0) {
        // 当没有读者后，解放写锁
        V(&cache->mutex_write);
    }
    V(&cache->mutex_reader);

    return success;
}

void writer(Cache *cache, char *uri, char *buf) {
    printf("%s\n","进入缓存的writer函数");
    printf("传入的uri为 %s\n", uri);
    P(&cache->mutex_write);
    strcpy(cache->data[cache->current_ptr].uri, uri);
    strcpy(cache->data[cache->current_ptr].obj, buf);
    printf("cache uri is %s\n", cache->data[cache->current_ptr].uri);
    printf("obj is %s\n", cache->data[cache->current_ptr].obj);
    cache->current_ptr = (cache->current_ptr + 1) % MAX_CACHELINE_NUM;
    V(&cache->mutex_write);
}

void delete_cache(Cache *cache) {
    free(cache);
}
