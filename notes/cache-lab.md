## 1 Part A
需要在csim.c中写一个cache的simulator，使用LRU（least-recently used）替换原则，即替换最近最少使用的内存行。其以valgrind的memory trace作为输入，输出cache中的hit、miss和eviction总数。

### 1.1 需求分析
根据书本的知识，高速缓存块基本如下所示，
<div align=center>
  <img src="/img/cache.jpg" width = 80% height = 80% />
  <br>
  <div style="color:orange;
    display: inline-block;
    color: #999;
    padding: 2px;">图1 高速缓存组织</div>
</div>

1. 由于模拟器必须适应不同的s, E, b，所以数据结构必须动态申请（malloc系列）；
2. 测试数据中以“I”开头的是对指令缓存（i-cache）进行读写，我们直接忽略这些行；
3. 此次实验假设内存全部对齐，即数据不会跨越block，所以测试数据里面的数据大小可以忽略；
4. 由于没有实际的数据存储，所以我们不用实现高速缓存行（cache line）中的数据块部分；
5. 实验要求使用LRU原则，即没有空模块（valid为0）时替换最早使用的那一个line，所以我们需要记录当前缓存行的时间参量，每次写入的时候更新该变量；

综上所述，我们给出以下结构体作为高速缓存行的基本结构，cache_line_t表示一个高速缓存行，cache_set_t表示一个高速缓存组，cache_t表示一个高速缓存。valid_bit表示有效位，只有一个bit，0表示无效，1表示有效，这里用int表示；tag_bits表示标记位；timestamp表示时间戳，当然，这里为了简便，直接用数值从0开始。
``` c
typedef struct
{
   int valid_bit;   // 有效标志位
   int tag_bits;    // 标记位
   int timestamp;   // 时间戳
}cache_line_t, *cache_set_t, **cache_t;
```

### 1.2 基本流程
设计代码的基本流程如下所示。
<div align=center>
  <img src="/img/cache-pro.jpg" width = 100% height = 100% />
  <br>
  <div style="color:orange;
    display: inline-block;
    color: #999;
    padding: 2px;">图2 基本流程</div>
</div>

### 1.3 代码
基本的代码如下：
``` c
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
```

运行结果如下：
``` bash
./test-csim 
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27

TEST_CSIM_RESULTS=27
```

## 2 Part B
Part B部分是优化矩阵转置，代码中给出了最简单的转置实现：
``` c
for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
        tmp = A[i][j];
        B[j][i] = tmp;
    }
}
```
这个部分要求去完成trans.c文件中的transpose_submit函数，实现矩阵的转置。要求如下：
1. 只准使用不超过12个int类型的局部变量，不允许使用long类型规避；
2. 不允许使用递归；
3. 如果使用辅助函数，则在辅助函数和顶级转置函数之间，一次最多不能有12个局部变量在堆栈上；
4. 不允许修改数组A，但是可以对数组B做任何事情；
5. 不允许在代码中定于任何数组或者使用malloc系列函数；

已经给定的cache参数是 s = 5，b = 5 ，E = 1。那么 cache 的大小就是 32 组，每组 1 行， 每行可存储 32 字节的数据。

### 2.1 32×32
在这个情况下，一行是32个int，也就是占据4个block，所以cache可以存8行，所以我们以8为单位进行分块，代码如下：
``` c
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        for (int i = 0; i < N; i += 8)
            for (int j = 0; j < M; j += 8) 
                for (int m = i; m < i + 8; m++)
                    for (int n = j; n < j + 8; n++)
                        B[n][m] = A[m][n];
    }
}
```
运行结果如下，可以看到距离满分的300还有距离。
``` bash
$ ./test-trans -N 32 -M 32

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:1710, misses:343, evictions:311

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:870, misses:1183, evictions:1151

Summary for official submission (func 0): correctness=1 misses=343

TEST_TRANS_RESULTS=1:343
```

以上距离满分的距离是是因为**A 和 B 中相同位置的元素是映射在同一 cache line 上的**，当A和B中的对角线位置时，譬如我们读取A<sub>ii</sub>处数据时，接下来会放到B<sub>ii</sub>处，因为二者映射的是同一行，使得第i行被替换成B的，但是随后又替换成A的，故而多造成了两次miss，所以我们修改代码如下，一次性读完一行，以减少不命中率，代码如下：
``` c
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        for (int i = 0; i < N; i += 8)
            for (int j = 0; j < M; j += 8) 
                for (int m = i; m < i + 8; m++) {
                    int v1 = A[m][j];
                    int v2 = A[m][j+1];
                    int v3 = A[m][j+2];
                    int v4 = A[m][j+3];
                    int v5 = A[m][j+4];
                    int v6 = A[m][j+5];
                    int v7 = A[m][j+6];
                    int v8 = A[m][j+7];
                    B[j][m] = v1;
                    B[j+1][m] = v2;
                    B[j+2][m] = v3;
                    B[j+3][m] = v4;
                    B[j+4][m] = v5;
                    B[j+5][m] = v6;
                    B[j+6][m] = v7;
                    B[j+7][m] = v8;
                }
    }
}
```
运行结果如下：
``` bash
$ ./test-trans -N 32 -M 32 

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:1766, misses:287, evictions:255

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:870, misses:1183, evictions:1151

Summary for official submission (func 0): correctness=1 misses=287

TEST_TRANS_RESULTS=1:287
```

### 2.2 64×64
#### 2.2.1 8×8分块
对 64 x 64 的矩阵来说，每行有 64 个 int，则 cache 只能存矩阵的 4 行了，所以如果使用 8x8 的分块，一定会在写 B 的时候造成冲突，因为映射到了相同的块。看一下 8x8 的分块能达到多少 miss。
``` c
if (M == 64 && N == 64) {
    for (int i = 0; i < N; i += 8)
        for (int j = 0; j < M; j += 8) 
            for (int m = i; m < i + 8; m++) {
                int v1 = A[m][j];
                int v2 = A[m][j+1];
                int v3 = A[m][j+2];
                int v4 = A[m][j+3];
                int v5 = A[m][j+4];
                int v6 = A[m][j+5];
                int v7 = A[m][j+6];
                int v8 = A[m][j+7];
                B[j][m] = v1;
                B[j+1][m] = v2;
                B[j+2][m] = v3;
                B[j+3][m] = v4;
                B[j+4][m] = v5;
                B[j+5][m] = v6;
                B[j+6][m] = v7;
                B[j+7][m] = v8;
            }
}
```
而结果也达到了惊人的4600多次：
``` bash
$ ./test-trans -N 64 -M 64 

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:3586, misses:4611, evictions:4579

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:3474, misses:4723, evictions:4691

Summary for official submission (func 0): correctness=1 misses=4611

TEST_TRANS_RESULTS=1:4611
```

#### 2.2.2 4×4分块
代码如下：
``` c
    if (M == 64 && N == 64) {
        for (int i = 0; i < N; i += 4)
            for (int j = 0; j < M; j += 4) 
                for (int m = i; m < i + 4; m++) {
                    int v1 = A[m][j];
                    int v2 = A[m][j+1];
                    int v3 = A[m][j+2];
                    int v4 = A[m][j+3];
                    B[j][m] = v1;
                    B[j+1][m] = v2;
                    B[j+2][m] = v3;
                    B[j+3][m] = v4;
                }
    }
```
运行结果也达到了1699次，还是达不到要求。
``` bash
$ ./test-trans -N 64 -M 64 

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:6498, misses:1699, evictions:1667

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:3474, misses:4723, evictions:4691

Summary for official submission (func 0): correctness=1 misses=1699

TEST_TRANS_RESULTS=1:1699
```

#### 2.2.3 多次分块
这里我是参考网络的方法去做的，代码如下，分析过程直接看[李秋豪](https://www.cnblogs.com/liqiuhao/p/8026100.html?utm_source=debugrun&utm_medium=referral)。
``` c
for (int i = 0; i < N; i += 8)
{
    for (int j = 0; j < M; j += 8)
    {
        for (int k = i; k < i + 4; ++k)
        {
        /* 读取1 2，暂时放在左下角1 2 */
            int temp_value0 = A[k][j];
            int temp_value1 = A[k][j+1];
            int temp_value2 = A[k][j+2];
            int temp_value3 = A[k][j+3];
            int temp_value4 = A[k][j+4];
            int temp_value5 = A[k][j+5];
            int temp_value6 = A[k][j+6];
            int temp_value7 = A[k][j+7];
          
            B[j][k] = temp_value0;
            B[j+1][k] = temp_value1;
            B[j+2][k] = temp_value2;
            B[j+3][k] = temp_value3;
          /* 逆序放置 */
            B[j][k+4] = temp_value7;
            B[j+1][k+4] = temp_value6;
            B[j+2][k+4] = temp_value5;
            B[j+3][k+4] = temp_value4;
        }
         for (int l = 0; l < 4; ++l)
        {
           /* 按列读取 */
            int temp_value0 = A[i+4][j+3-l];
            int temp_value1 = A[i+5][j+3-l];
            int temp_value2 = A[i+6][j+3-l];
            int temp_value3 = A[i+7][j+3-l];
            int temp_value4 = A[i+4][j+4+l];
            int temp_value5 = A[i+5][j+4+l];
            int temp_value6 = A[i+6][j+4+l];
            int temp_value7 = A[i+7][j+4+l];
           
           /* 从下向上按行转换2到3 */
            B[j+4+l][i] = B[j+3-l][i+4];
            B[j+4+l][i+1] = B[j+3-l][i+5];
            B[j+4+l][i+2] = B[j+3-l][i+6];
            B[j+4+l][i+3] = B[j+3-l][i+7];
           /* 将3 4放到正确的位置 */
            B[j+3-l][i+4] = temp_value0;
            B[j+3-l][i+5] = temp_value1;
            B[j+3-l][i+6] = temp_value2;
            B[j+3-l][i+7] = temp_value3;
            B[j+4+l][i+4] = temp_value4;
            B[j+4+l][i+5] = temp_value5;
            B[j+4+l][i+6] = temp_value6;
            B[j+4+l][i+7] = temp_value7;
        } 
    }
}
```
执行结果如下，达到了满分要求。
``` bash
$ ./test-trans -N 64 -M 64 

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:9002, misses:1243, evictions:1211

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:3474, misses:4723, evictions:4691

Summary for official submission (func 0): correctness=1 misses=1243

TEST_TRANS_RESULTS=1:1243
```

### 2.3 61×67
这题的要求比较宽松，所以我们直接用8×8分块，因为不是很对称，还需要对多余的部分进行一下处理，代码和得分如下：
``` c
if (M == 61 && N == 67) {
	int i, j, v1, v2, v3, v4, v5, v6, v7, v8;
	int n = N / 8 * 8;
	int m = M / 8 * 8;
	for (j = 0; j < m; j += 8)
		for (i = 0; i < n; ++i) {
			v1 = A[i][j];
			v2 = A[i][j+1];
			v3 = A[i][j+2];
			v4 = A[i][j+3];
			v5 = A[i][j+4];
			v6 = A[i][j+5];
			v7 = A[i][j+6];
			v8 = A[i][j+7];
			
			B[j][i] = v1;
			B[j+1][i] = v2;
			B[j+2][i] = v3;
			B[j+3][i] = v4;
			B[j+4][i] = v5;
			B[j+5][i] = v6;
			B[j+6][i] = v7;
			B[j+7][i] = v8;
		}
	for (i = 0; i < N; ++i)
		for (j = m; j < M; ++j) {
			v1 = A[i][j];
			B[j][i] = v1;
		}
	for (i = n; i < N; ++i)
		for (j = 0; j < M; ++j) {
			v1 = A[i][j];
			B[j][i] = v1;
		}
}
```
``` bash
$ ./test-trans -N 67 -M 61 

Function 0 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 0 (Transpose submission): hits:6312, misses:1897, evictions:1865

Function 1 (2 total)
Step 1: Validating and generating memory traces
Step 2: Evaluating performance (s=5, E=1, b=5)
func 1 (Simple row-wise scan transpose): hits:3756, misses:4423, evictions:4391

Summary for official submission (func 0): correctness=1 misses=1897

TEST_TRANS_RESULTS=1:1897
```

## 总结
driver.py运行结果：
``` bash
$ ./driver.py 
Part A: Testing cache simulator
Running ./test-csim
                        Your simulator     Reference simulator
Points (s,E,b)    Hits  Misses  Evicts    Hits  Misses  Evicts
     3 (1,1,1)       9       8       6       9       8       6  traces/yi2.trace
     3 (4,2,4)       4       5       2       4       5       2  traces/yi.trace
     3 (2,1,4)       2       3       1       2       3       1  traces/dave.trace
     3 (2,1,3)     167      71      67     167      71      67  traces/trans.trace
     3 (2,2,3)     201      37      29     201      37      29  traces/trans.trace
     3 (2,4,3)     212      26      10     212      26      10  traces/trans.trace
     3 (5,1,5)     231       7       0     231       7       0  traces/trans.trace
     6 (5,1,5)  265189   21775   21743  265189   21775   21743  traces/long.trace
    27


Part B: Testing transpose function
Running ./test-trans -M 32 -N 32
Running ./test-trans -M 64 -N 64
Running ./test-trans -M 61 -N 67

Cache Lab summary:
                        Points   Max pts      Misses
Csim correctness          27.0        27
Trans perf 32x32           8.0         8         287
Trans perf 64x64           8.0         8        1243
Trans perf 61x67          10.0        10        1897
          Total points    53.0        53
```

cache-lab的难度还是相当大的，特别是Part B部分，虽然分块使得代码难以阅读和理解，但是学习和理解这项技术还是很有趣的。
