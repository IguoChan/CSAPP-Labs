#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include "cachelab.h"

int verbose = 0;// verbose flag
int s = 0;      // 组索引位的数量
int E = 0;      // 每个组的行数；
int b = 0;      // 块偏移
int S = 0;      // 组数
char *t = NULL;    // 存储文件名
int hit_count = 0;
int miss_count = 0;
int eviction_count = 0; 

typedef struct
{
   int valid_bit;   // 有效标志位
   int tag_bits;    // 标记位
   int timestamp;   // 时间戳
}cache_line_t, *cache_set_t, **cache_t;

cache_t cache = NULL;

void printUsage()
{
    printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n"

            "Examples:\n"
            "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void parseArgs(int argc, char **argv)
{
    int opt;
    while((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt)
        {
        case 'h':
            printUsage();
            exit(1);
        case 'v':
            verbose = 1;
            break;
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
            t = (char*)malloc(strlen(optarg) + 1);
            strncpy(t, optarg, strlen(optarg) + 1);
            break;        
        default:
            printUsage();
            exit(1);
        }
    }
}

void init_cache()
{
    cache = (cache_t)malloc(sizeof(cache_set_t) * S);
    for (int i = 0; i < S; i++) {
        cache[i] = (cache_set_t)malloc(sizeof(cache_line_t) * E);
        for (int j = 0; j < E; j++) {
            cache[i][j].valid_bit = 0;
            cache[i][j].tag_bits = -1;
            cache[i][j].timestamp = -1;
        }
    }
}

void free_cache()
{
    for (int i = 0; i < S; i++)
        free(cache[i]);
    free(cache);
}

/*
Return value
0 - hit
1 - miss
2 - evict
*/
int hitMissEviction(unsigned int address)
{
    unsigned int setIndex = address >> b & ((1 << s) - 1);
    int tag = address >> (s + b);
    cache_set_t cacheSet = cache[setIndex];
    int max_timestamp = -1;
    int max_timestamp_index = -1;

    for (int i = 0; i < E; i++) {
        if(cacheSet[i].tag_bits == tag) {
            cacheSet[i].timestamp = 0;
            hit_count++;
            return 0;
        }
    }

    for (int i = 0; i < E; i++) {
        if (!cacheSet[i].valid_bit) {
            cacheSet[i].valid_bit = 1;
            cacheSet[i].tag_bits = tag;
            cacheSet[i].timestamp = 0;
            miss_count++;
            return 1;
        }
    }

    for (int i = 0; i < E; i++) {
        if(cacheSet[i].timestamp > max_timestamp) {
            max_timestamp = cacheSet[i].timestamp;
            max_timestamp_index = i;
        }
    }
    cacheSet[max_timestamp_index].tag_bits = tag;
    cacheSet[max_timestamp_index].timestamp = 0;
    eviction_count++;
    miss_count++;
    return 2;
}

void update_timestamp()
{
    for (int i = 0; i < S; i++)
        for (int j = 0; j< E; j++)
            if (cache[i][j].valid_bit)
                cache[i][j].timestamp++;
}

void simulate()
{   
    char operation;         // 命令开头的 I L M S
    unsigned int address;   // 地址参数
    int size;               // 大小
    int ret;
    FILE *fp = fopen(t, "r");
    if (fp == NULL) {
        return;
    }
    S = 1 << s; // S = 2^s
    init_cache(); 

    while (fscanf(fp, " %c %xu,%d\n", &operation, &address, &size) > 0) {
        switch (operation)
        {
        case 'L':
            ret = hitMissEviction(address);
            break;
        case 'M':
            ret = hitMissEviction(address); // 再试一遍, fall througth
        case 'S':
            ret = hitMissEviction(address);
            break;
        }
        update_timestamp();
    }
    if (verbose) {
        switch (ret)
        {
        case 0:
            printf("%c %x,%d hit\n", operation, address, size);
            break;
        case 1:
            printf("%c %x,%d miss\n", operation, address, size);
            break;
        case 2:
            printf("%c %x,%d eviation\n", operation, address, size);
            break;        
        default:
            break;
        }
    }
    free_cache();
    fclose(fp);
}

int main(int argc, char **argv)
{
    parseArgs(argc, argv);
    if (s <= 0 || E <= 0 || b <= 0 || t == NULL) {
        printf("wrong input param!");
        return -1;
    }
    simulate();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
