#include "cachelab.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

typedef struct {
    int valid;      // 有效位,1有效，0无效
    int tag;        // 标记
    int timetamp;   // 时间戳
    char* block;    //  缓存块
} line_t;

typedef struct {
    int S;              // 组的数量
    int E;              // 每组的行数
    int B;              // 块的大小
    line_t ***lines;    // 行的数组
} cache_t;

static cache_t Cache;
static int Hits = 0, Misses = 0, Evicts = 0;
static const int max_time = INT_MAX;

void initLines() {
    // line_t ** : 指向一个组 
    // line_t *  : 指向一个行

    // 初始化组数组
    Cache.lines = (line_t ***)malloc(Cache.S * sizeof(line_t **));
    for (int i=0; i < Cache.S; ++i) {
        Cache.lines[i] = (line_t **)malloc(Cache.E * sizeof(line_t *));
    }
    // 初始化每个组中的行数组
    for (int i=0; i < Cache.S; ++i) {
        for (int j=0; j < Cache.E; ++j) {
            Cache.lines[i][j] = (line_t *)malloc(sizeof(line_t));
        }
    }
    // 初始化每个组中的元数据和块
    for (int i=0; i < Cache.S; ++i) {
        for (int j=0; j < Cache.E; ++j) {
            Cache.lines[i][j]->valid = 0;
            Cache.lines[i][j]->tag = -1;
            Cache.lines[i][j]->timetamp = max_time;
            Cache.lines[i][j]->block = (char *)malloc(sizeof(char) * Cache.B);
        }
    }
}

void initCache(int s, int e, int b) {
    Cache.S = 1 << s;
    Cache.E = e;
    Cache.B = 1 << b;

    initLines();
}


void freeCache() {
    // 清理每个block指针
    for (int i=0; i < Cache.S; ++i) {
        for (int j = 0; j < Cache.E; ++j) {
            free(Cache.lines[i][j]->block);
        }
    }
    // 清理行
    for (int i=0; i < Cache.S; ++i) {
        for (int j = 0; j < Cache.E; ++j) {
            free(Cache.lines[i][j]);
        }
    }
    // 清理组
    for (int i=0; i < Cache.S; ++i) {
        free(Cache.lines[i]);
    }
    // 清理lines
    free(Cache.lines);
}

/**
 * 若查找成功，返回该行在组中的序号，否则返回-1
*/
int getLine(int set_id, int tag) {
    for (int i=0; i<Cache.E; ++i) {
        if (Cache.lines[set_id][i]->valid && (Cache.lines[set_id][i]->tag == tag)) {
            return i;
        }
    }
    return -1;
}

// 获取该组中LRU的行号
int getLRULine(int set_id) {
    int line_index = 0, min_stamp = max_time;
    for (int i=0; i<Cache.E; ++i) {
        if (Cache.lines[set_id][i]->timetamp < min_stamp) {
            line_index = i;
            min_stamp = Cache.lines[set_id][i]->timetamp;
        }
    }
    return line_index;
}

// 获取组set_id中的空行行号
int getEmptyLine(int set_id) {
    for (int i=0; i<Cache.E; ++i) {
        if (Cache.lines[set_id][i]->valid == 0) {
            return i;
        }
    }
    return -1;
}

// 更新某一行
void updateLine(int set_id, int line_id, int tag) {
    // 更新当前行
    Cache.lines[set_id][line_id]->valid = 1;
    Cache.lines[set_id][line_id]->tag = tag;
    Cache.lines[set_id][line_id]->timetamp = max_time;

    // 其他行倒计时
    for (int i=0; i < Cache.S; ++i) {
        for (int j = 0; j < Cache.E; ++j) {
            Cache.lines[i][j]->timetamp--;
        }
    }
}

// 访问Cache
void accessCache(int set_id, int tag) {
    int line_id = getLine(set_id, tag);
    if (line_id == -1) {    // 缓存未命中
        Misses++;
        line_id = getEmptyLine(set_id);
        if (line_id==-1) {  // 组中没有空行，需要缓存替换
            Evicts++;
            line_id = getLRULine(set_id);
        }
        updateLine(set_id, line_id, tag);
    } else {
        Hits++;
        updateLine(set_id, line_id, tag);
    }
}

// 通过追踪文件模拟对整个缓存的使用
void simTrace(int s, int E, int b, char *filename) {
    FILE *file_ptr;
    file_ptr = fopen(filename, "r");
    if (file_ptr==NULL) {
        printf("找不到追踪文件%s\n", filename);
        exit(1);
    }
    // 对应trace中的三个元素
    char operation;
    unsigned address;
    int size;
    // 从追踪文件中读取访问数据
    while (fscanf(file_ptr, " %c %x,%d", &operation, &address, &size) > 0) {
        int tag = address  >> (s+b);
        int set_id = (address >> b) & ((unsigned)(-1) >> (8*sizeof(unsigned)-s));
        switch (operation) {
        case 'M':   // 修改需要两次访存：取出和重新保存
            accessCache(set_id, tag);
            accessCache(set_id, tag);
            break;
        case 'L':
            accessCache(set_id, tag);
            break;
        case 'S':
            accessCache(set_id, tag);
            break;
        }
    }
    fclose(file_ptr);
}



int main(int argc, char **argv)
{
    char opt;
    int s,E,b;
    char filename[100];

    while ((opt=getopt(argc, argv, "hvs:E:b:t:"))!=-1) {
        switch (opt)
        {
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            strcpy(filename, optarg);
            break;
        default:
            printf("Cannot read the flag %c\n", opt);
            exit(1);
        }
    }

    initCache(s,E,b);
    simTrace(s,E,b,filename);
    printSummary(Hits, Misses, Evicts);
    freeCache();

    return 0;
}
