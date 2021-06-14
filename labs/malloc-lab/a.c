#include <stdlib.h>
#include <stdio.h>

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


int main()
{
	char *a = (char *)malloc(1);
	int an = malloc_usable_size(a);
	printf("an = %d\n", an);
	char *a1 = (char *)malloc(9);
	int an1 = malloc_usable_size(a1);
	printf("an1 = %d\n", an1);
	char *a2 = (char *)malloc(10);
	int an2 = malloc_usable_size(a2);
	printf("an2 = %d\n", an2);
	free(a);
	free(a1);
	free(a2);
	free(NULL);
}
