/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*定义一些方便使用的宏*/
#define WSIZE 4             // 字大小
#define DSIZE 8             // 双字大小
#define CHUNKSIZE (1<<12)   // 一次拓展堆空间大小，单位字节
#define MAX(x, y) ((x)>(y)?(x):(y))

#define PACK(size, alloc) ((size) | (alloc))        // 设置头/脚部的方法

#define GET(p) (*(unsigned int *)p)                 // 读p指向的字
#define PUT(p, val) (*(unsigned int *)(p) = (val))  // 写p指向的字

#define GET_SIZE(p) (GET(p) & ~0x7) // 获得块的大小，这里默认p指向头部或脚部
#define GET_ALLOC(p) (GET(p) & 0x1) // 获得块是否已分配，这里默认p指向头部或脚部

#define HDRP(bp) ((char *)(bp) - WSIZE)     // 获取bp指向块的头部
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)    // 获取bp指向块的尾部

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // 获取bp下一个块的块指针
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // 获取bp上一个块的块指针

static char *heap_listp;     // 私有全局指针变量，永远是序言块的块指针

static void *extend_heap(size_t words); // 用一个新的空闲块拓展堆
static void *coalesce(void *bp);        // 合并空闲空间
static void place(void *bp, size_t asize);   // 将申请空间放入某空闲块
static void *find_fit(size_t asize);        // 首次匹配

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // 分配失败
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) {
        return -1;
    }

    // 清空填充块
    PUT(heap_listp, 0);
    // 设置序言块
    PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));
    // 设置结尾块
    PUT(heap_listp+(3*WSIZE), PACK(0, 1));

    // 将heap_listp设置为序言块指针
    heap_listp += (2*WSIZE);

    // 用一个新的空闲块拓展堆
    if (extend_heap(CHUNKSIZE/WSIZE)==NULL) {
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;   // 实际需要分配的空间
    size_t extend_size; // 如果需要拓展堆，则需要的拓展空间
    char *bp;

    // 不合法的输入
    if (size==0) {
        return NULL;
    }

    // 保证双字对齐的要求
    if (size <= DSIZE) {    // 最小块的限制
        asize = 2*DSIZE;
    } else {
        // 实际需要的空间 = 申请空间 + 头部和脚部，然后对双字向上取整以对齐
        asize = DSIZE * ((size + (DSIZE)+(DSIZE-1)) / DSIZE);
    }

    // 采用首次匹配的方式找到合适的空闲块
    if ((bp=find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // 如果没有足够大的空闲块，则拓展堆空间
    extend_size = MAX(asize, CHUNKSIZE);
    if ((bp=extend_heap(extend_size/WSIZE)) == NULL) {
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    // 获取当前快的大小
    size_t size = GET_SIZE(HDRP(ptr));

    // 设置当前块状态为空闲
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(HDRP(ptr), PACK(size, 0));

    // 合并前后的空闲块
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    copySize = GET_SIZE(HDRP(oldptr));
    if (size < copySize)
      copySize = size;

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}


/**
 * 用一个空闲块拓展堆空间
*/
static void *extend_heap(size_t words) {
    char * bp;
    size_t size;

    // 申请内存空间，注意保证双字对齐
    size = (words%2) ? (words+1)*WSIZE : (words * WSIZE);
    if ((bp = mem_sbrk(size)) == (void *)-1) {
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));   // 设置新空闲块的头
    PUT(FTRP(bp), PACK(size, 0));   // 设置新空闲块的脚
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));    // 设置新的堆结尾块

    // 合并空闲块
    return coalesce(bp);
}

/**
 * 合并bp指向块的前后空闲空间（默认当前块空闲）
*/
static void *coalesce(void *bp) {
    // 判断前后块的分配情况
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { // case 1：前后块都已经被分配
        return bp;
    }
    else if (prev_alloc && !next_alloc) {   // case 2：后面的块是空闲的
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) {   // case 3：前面的块是空闲的
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {  // case 4: 前后的块都是空闲的
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + 
            GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/**
 * 将申请的空间放入空闲块中
*/
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));  // 空闲块本身的大小
    size_t rsize = csize - asize;       // 分配空间后剩余大小

    if ((rsize) >= 2*DSIZE) {   // 如果剩下的空间足够再形成一个块
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(rsize, 0));
        PUT(FTRP(bp), PACK(rsize, 0));
    } else {    // 剩下的空间已经不够再形成一个新块，则分配空闲块的所有
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/**
 * 首次适配的查找方式
*/
static void *find_fit(size_t asize) {
    void *bp;
    // 从头遍历堆空间中的所有块
    for (bp = heap_listp; GET_SIZE(HDRP(bp))>0; bp = NEXT_BLKP(bp)) {
        if ((GET_SIZE(HDRP(bp)) >= asize) && (!GET_ALLOC(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;
}








