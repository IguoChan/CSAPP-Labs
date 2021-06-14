## 1. 书本代码
其实书本上已经实现了部分的代码，我们首先将书本的代码实现，看看分数是多少？有关分数的评定，下载的文件中并没有给到，大家也可以去[]()中拿到相关trace文件，以作评分用。

``` c
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
    "iguochan",
    /* First member's full name */
    "IguoChan",
    /* First member's email address */
    "iguochan@foxmail.com",
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

/* Basic constants and macros */
#define WSIZE               4
#define DSIZE               8
#define CHUNKSIZE           (1 << 12)

#define MAX(x, y)           ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)   ((size) | (alloc))

#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

#define HDRP(bp)            ((char *)(bp) - WSIZE)
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void place(void *bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) return NULL;

    if (size <= DSIZE) 
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = first_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;    
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {                     /* Case 1 */
        /* Do nothing */
    } else if (prev_alloc && !next_alloc) {             /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {             /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                            /* Case 4 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

static void *first_fit(size_t asize)
{
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
            return bp;
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    
    if ((size - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize,1));
        PUT(FTRP(bp), PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size - asize,0));
        PUT(FTRP(bp), PACK(size - asize,0));
    } else {
        PUT(HDRP(bp), PACK(size,1));
        PUT(FTRP(bp), PACK(size,1));
    }
}
```

分数如下：
``` bash
$ ./mdriver -t ./traces -V
Team Name:iguochan
Member 1 :IguoChan:iguochan@foxmail.com
Using default tracefiles in ./traces/
Measuring performance with gettimeofday().
...

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.009994   570
 1       yes   99%    5848  0.008913   656
 2       yes   99%    6648  0.013963   476
 3       yes  100%    5380  0.011272   477
 4       yes   66%   14400  0.000108133705
 5       yes   92%    4800  0.010015   479
 6       yes   92%    4800  0.009609   500
 7       yes   55%   12000  0.171066    70
 8       yes   51%   24000  0.322512    74
 9       yes   27%   14401  0.072254   199
10       yes   34%   14401  0.003717  3874
Total          74%  112372  0.633421   177

Perf index = 44 (util) + 12 (thru) = 56/100
```

## 2. 用next_fit代替first_fit
我们用pre_listp指针指向上次分配的点，这样就不用每次从头去找空闲块了，整体代码如下：
``` c
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
    "iguochan",
    /* First member's full name */
    "IguoChan",
    /* First member's email address */
    "iguochan@foxmail.com",
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

/* Basic constants and macros */
#define WSIZE               4
#define DSIZE               8
#define CHUNKSIZE           (1 << 12)

#define MAX(x, y)           ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)   ((size) | (alloc))

#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

#define HDRP(bp)            ((char *)(bp) - WSIZE)
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

static char *heap_listp;
static char *pre_listp;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *first_fit(size_t asize);
static void *next_fit(size_t asize);
static void place(void *bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);
    pre_listp = heap_listp;

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0) return NULL;

    if (size <= DSIZE) 
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    if ((bp = next_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;    
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = GET_SIZE(HDRP(newptr));
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {                     /* Case 1 */
        /* Do nothing */
    } else if (prev_alloc && !next_alloc) {             /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {             /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                            /* Case 4 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    pre_listp = bp;
    return bp;
}

static void *next_fit(size_t asize)
{
    void *bp;
    for (bp = pre_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            pre_listp = bp;
            return bp;
        }
    }
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            pre_listp = bp;
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    
    if ((size - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize,1));
        PUT(FTRP(bp), PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size - asize,0));
        PUT(FTRP(bp), PACK(size - asize,0));
    } else {
        PUT(HDRP(bp), PACK(size,1));
        PUT(FTRP(bp), PACK(size,1));
    }
    pre_listp = bp;
}
```
分数如下，一下子提高到了83分。
``` bash
$ ./mdriver -t ./traces -V
Team Name:iguochan
Member 1 :IguoChan:iguochan@foxmail.com
Using default tracefiles in ./traces/
Measuring performance with gettimeofday().
...

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   90%    5694  0.003638  1565
 1       yes   93%    5848  0.002041  2865
 2       yes   94%    6648  0.004940  1346
 3       yes   96%    5380  0.005217  1031
 4       yes   66%   14400  0.000108133581
 5       yes   89%    4800  0.007406   648
 6       yes   87%    4800  0.007437   645
 7       yes   55%   12000  0.018366   653
 8       yes   51%   24000  0.011100  2162
 9       yes   26%   14401  0.072281   199
10       yes   34%   14401  0.003841  3749
Total          71%  112372  0.136373   824

Perf index = 43 (util) + 40 (thru) = 83/100
```

## 3. 分离的空闲链表
要想继续提高分配器的性能，需要用到课本内容9.9.13-9.9.14的知识。我们需要构建一个分离空闲链表，具体参考的是[CS:APP3e 深入理解计算机系统_3e MallocLab实验](https://www.cnblogs.com/liqiuhao/p/8252373.html)。具体代码如下：
``` c
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
    "iguochan",
    /* First member's full name */
    "IguoChan",
    /* First member's email address */
    "iguochan@foxmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
// #define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define ALIGN(size) ((((size) + (ALIGNMENT-1)) / (ALIGNMENT)) * (ALIGNMENT))


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE               4
#define DSIZE               8
#define CHUNKSIZE           (1 << 12)
#define INITCHUNKSIZE       (1 << 6)
#define LISTMAX             16

#define MAX(x, y)           ((x) > (y) ? (x) : (y))

#define PACK(size, alloc)   ((size) | (alloc))

#define GET(p)              (*(unsigned int *)(p))
#define PUT(p, val)         (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)         (GET(p) & ~0x7)
#define GET_ALLOC(p)        (GET(p) & 0x1)

#define HDRP(bp)            ((char *)(bp) - WSIZE)
#define FTRP(bp)            ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)       ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)       ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define PRED_PTR(bp)       ((char *)(bp))
#define SUCC_PTR(bp)       ((char *)(bp) + WSIZE)

#define PRED(bp)           (*(char **)(bp))
#define SUCC(bp)           (*(char **)(SUCC_PTR(bp)))

#define SET_PTR(p, bp)     ((*(unsigned int *)(p) = (unsigned int)(bp)))

void *segregated_free_lists[LISTMAX];

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void place(void *bp, size_t asize);
static void insert_node(void *bp, size_t size);
static void delete_node(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int listnumber;
    char *heap;

    for (listnumber = 0; listnumber < LISTMAX; listnumber++) {
        segregated_free_lists[listnumber] = NULL;
    }

    if ((heap = mem_sbrk(4*WSIZE)) == (void*)-1)
        return -1;
    PUT(heap, 0);
    PUT(heap + (WSIZE), PACK(DSIZE, 1));
    PUT(heap + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap + (3 * WSIZE), PACK(0, 1));

    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp = NULL;
    int listnumber = 0;
    size_t searchsize = size;

    if (size == 0) return NULL;

    if (size <= DSIZE) 
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    while (listnumber < LISTMAX)
    {
        /* 寻找对应链 */
        if (((searchsize <= 1) && (segregated_free_lists[listnumber] != NULL)))
        {
            bp = segregated_free_lists[listnumber];
            /* 在该链寻找大小合适的free块 */
            while ((bp != NULL) && ((asize > GET_SIZE(HDRP(bp)))))
            {
                bp = PRED(bp);
            }
            /* 找到对应的free块 */
            if (bp != NULL)
                break;
        }

        searchsize >>= 1;
        listnumber++;
    }

    /* No fit found. Get more memory and place the block */
    if (bp == NULL) {
        extendsize = MAX(asize,CHUNKSIZE);
        if ((bp = extend_heap(extendsize)) == NULL)
            return NULL;
    }
    place(bp, asize);
    return bp;    
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    if (bp == NULL) {
        return;
    }

    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    insert_node(bp, size);
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *new_block = ptr;
    int remainder;

    if (size == 0)
        return NULL;

    /* 内存对齐 */
    if (size <= DSIZE)
    {
        size = 2 * DSIZE;
    }
    else
    {
        size = ALIGN(size + DSIZE);
    }

    /* 如果size小于原来块的大小，直接返回原来的块 */
    if ((remainder = GET_SIZE(HDRP(ptr)) - size) >= 0)
    {
        return ptr;
    }
    /* 否则先检查地址连续下一个块是否为free块或者该块是堆的结束块，因为我们要尽可能利用相邻的free块，以此减小“external fragmentation” */
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr))))
    {
        /* 即使加上后面连续地址上的free块空间也不够，需要扩展块 */
        if ((remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0)
        {
            if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL)
                return NULL;
            remainder += MAX(-remainder, CHUNKSIZE);
        }

        /* 删除刚刚利用的free块并设置新块的头尾 */
        delete_node(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(size + remainder, 1));
        PUT(FTRP(ptr), PACK(size + remainder, 1));
    }
    /* 没有可以利用的连续free块，而且size大于原来的块，这时只能申请新的不连续的free块、复制原块内容、释放原块 */
    else
    {
        new_block = mm_malloc(size);
        memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
        mm_free(ptr);
    }

    return new_block;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    size = ALIGN(words);
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    insert_node(bp, size);
    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {                     /* Case 1 */
        /* Do nothing */
        return bp;
    } else if (prev_alloc && !next_alloc) {             /* Case 2 */
        delete_node(bp);
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {             /* Case 3 */
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                            /* Case 4 */
        delete_node(bp);
        delete_node(PREV_BLKP(bp));
        delete_node(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    insert_node(bp, size);
    return bp;
}

static void place(void *bp, size_t asize)
{
    size_t size = GET_SIZE(HDRP(bp));
    delete_node(bp);
    
    if ((size - asize) >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize,1));
        PUT(FTRP(bp), PACK(asize,1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size - asize,0));
        PUT(FTRP(bp), PACK(size - asize,0));
        insert_node(bp, size-asize);
    } else {
        PUT(HDRP(bp), PACK(size,1));
        PUT(FTRP(bp), PACK(size,1));
    }
}

/* listnumber和search_size的对应
    |   listnumber  |   search_size |
    |       0       |       1       |
    |       1       |      2-3      |
    |       2       |      4-7      |
    |       3       |      8-15     |
    |       4       |     16-31     |
    |       5       |     32-63     |
    |       6       |     64-127    |
    |       7       |    128-255    |
    |       8       |    256-511    |
    |       9       |    512-1023   |
    |       10      |   1024-2047   |
    |       11      |   2048-4095   |
    |       12      |  4096-2^13-1  |
    |       13      |  2^13-2^14-1  |
    |       14      |  2^14-2^15-1  |
    |       15      |  2^15-2^16-1  |
 */

static void insert_node(void *bp, size_t size) {
    int listnumber = 0;
    size_t search_size = size;
    void *search_ptr = NULL;
    void *insert_ptr = NULL;

    /* 通过块的大小找到对应的链 */
    while (listnumber < LISTMAX - 1 && search_size > 1) {
        search_size >>= 1;
        listnumber++;
    }

    /* 找到对应的链后，在该链中继续寻找对应的插入位置，以此保持链中块由大到小的特性
       segregated_free_lists存储的是链表尾部的指针 */
    search_ptr = segregated_free_lists[listnumber];
    while (search_ptr != NULL && size > GET_SIZE(HDRP(search_ptr))) {
        insert_ptr = search_ptr;
        search_ptr = PRED(search_ptr);
    }

    if (search_ptr != NULL) {
        /* 1. ...->search->bp->insert->... 在中间插入*/
        if (insert_ptr != NULL)
        {
            SET_PTR(PRED_PTR(bp), search_ptr);
            SET_PTR(SUCC_PTR(search_ptr), bp);
            SET_PTR(SUCC_PTR(bp), insert_ptr);
            SET_PTR(PRED_PTR(insert_ptr), bp);
        }
        /* 2. ...->search->bp->NULL 在结尾插入*/
        /* 这种情形只能出现在没有进第二个while循环，也就是首次即不满足size > GET_SIZE(HDRP(search_ptr))，直接插在最后，更新segregated_free_lists */
        else
        {
            SET_PTR(PRED_PTR(bp), search_ptr);
            SET_PTR(SUCC_PTR(search_ptr), bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            segregated_free_lists[listnumber] = bp;
        }
    } else {
        /* 3. [listnumber]->bp->insert->... 在开头插入，但是后面有插入的 */
        if (insert_ptr != NULL) {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), insert_ptr);
            SET_PTR(PRED_PTR(insert_ptr), bp);
        }
        /* 4. [listnumber]->bp->NULL 该链表为空，首次插入，需更新segregated_free_lists */
        else 
        {
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            segregated_free_lists[listnumber] = bp;
        }
    }
    
}

static void delete_node(void *bp)
{
    int listnumber = 0;
    size_t size = GET_SIZE(HDRP(bp));

    /* 通过块的大小找到对应的链 */
    while ((listnumber < LISTMAX - 1) && (size > 1))
    {
        size >>= 1;
        listnumber++;
    }

    /* 通过块的大小找到对应的链 */
    if (PRED(bp) != NULL) {
        /* 1. ...->bp->... 在链表中间 */
        if (SUCC(bp) != NULL)
        {
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        }
        /* 2. ...->bp->NULL 在链表末尾 */
        else
        {
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            segregated_free_lists[listnumber] = PRED(bp);
        }
    } else {
       /* 3. ...->bp->... 在链表头，但是后续有链表 */
        if (SUCC(bp) != NULL)
        {
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        }
        /* 4. ...->bp->NULL 在链表头，且是唯一元素 */
        else
        {
            segregated_free_lists[listnumber] = NULL;
        }
    }
}
```
跑分如下：
``` bash
$$ ./mdriver -t traces -V
Team Name:iguochan
Member 1 :IguoChan:iguochan@foxmail.com
Using default tracefiles in traces/
Measuring performance with gettimeofday().
...

Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.000336 16967
 1       yes   99%    5848  0.000383 15277
 2       yes  100%    6648  0.000959  6930
 3       yes  100%    5380  0.000316 17025
 4       yes   99%   14400  0.000678 21226
 5       yes   96%    4800  0.001296  3704
 6       yes   96%    4800  0.001011  4749
 7       yes   55%   12000  0.000445 26966
 8       yes   51%   24000  0.001557 15410
 9       yes   99%   14401  0.000244 59142
10       yes   86%   14401  0.000226 63721
Total          89%  112372  0.007451 15082

Perf index = 53 (util) + 40 (thru) = 93/100
```
