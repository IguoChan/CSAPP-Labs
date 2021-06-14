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