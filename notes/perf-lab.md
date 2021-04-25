## 1 Rotate
我们首先查看原naive_rotate函数如下，其实进行的就是逆时针90°的旋转
``` c
#define RIDX(i,j,n) ((i)*(n)+(j))

void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
	    	dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}
```

### 1.1 调换i,j的顺序
在原来的naive_rotate的src是按照行访问，dst是按照列访问，我们将其改成src按列访问，dst按行访问，如下：
``` c
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
	    	dst[RIDX(i, j, dim)] = src[RIDX(j, dim-1-i, dim)];
}
```
其运行的结果如下所示，可以看到有很大的提升，原因在于之前每次内循环都得计算依次`dim-1-j`，而现在每次外循环才计算一次dim-1-i，大大减小了计算量。
``` bash
$ ./driver
Rotate: Version = naive_rotate: Naive baseline implementation:
Dim		        64	    128	     256	512	    1024	Mean
Your CPEs	    2.5	    4.1	     4.9	8.6	    11.2
Baseline CPEs	14.7	40.1	 46.4	65.9	94.5
Speedup		    5.9	    9.9	     9.5	7.6	    8.5	    8.2

Rotate: Version = rotate: Current working version:
Dim		        64	    128	    256	    512	    1024	Mean
Your CPEs	    2.2	    2.4	    3.1	    4.4	    6.7
Baseline CPEs	14.7	40.1	46.4	65.9	94.5
Speedup		    6.7	    17.0	15.2	15.1	14.2	13.0
```

### 1.2 循环展开
循环展开可以避免频繁地测试循环条件从而加快执行速度：
* writeup中图片尺寸是32的倍数，所以我们可以进行32路展开；
* 根据空间局部性原则，使内部循环步长短于外部循环；
所以我们给出以下代码：
```c
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i,j;
    dst+=(dim*dim-dim);
    for(i=0;i<dim;i+=32){
        for(j=0;j<dim;j++){
            dst[0]=src[0];
            dst[1]=src[dim];
            dst[2]=src[2*dim];
            dst[3]=src[3*dim];
            dst[4]=src[4*dim];
            dst[5]=src[5*dim];
            dst[6]=src[6*dim];
            dst[7]=src[7*dim];
            dst[8]=src[8*dim];
            dst[9]=src[9*dim];
            dst[10]=src[10*dim];
            dst[11]=src[11*dim];
            dst[12]=src[12*dim];
            dst[13]=src[13*dim];
            dst[14]=src[14*dim];
            dst[15]=src[15*dim];
            dst[16]=src[16*dim];
            dst[17]=src[17*dim];
            dst[18]=src[18*dim];
            dst[19]=src[19*dim];
            dst[20]=src[20*dim];
            dst[21]=src[21*dim];
            dst[22]=src[22*dim];
            dst[23]=src[23*dim];
            dst[24]=src[24*dim];
            dst[25]=src[25*dim];
            dst[26]=src[26*dim];
            dst[27]=src[27*dim];
            dst[28]=src[28*dim];
            dst[29]=src[29*dim];
            dst[30]=src[30*dim];
            dst[31]=src[31*dim];
            src++;
            dst-=dim;
        }
        src+=31*dim;
        dst+=dim*dim+32;
    }
}
```
执行结果如下
``` bash
$ ./driver
Rotate: Version = naive_rotate: Naive baseline implementation:
Dim		        64	    128	    256	    512	    1024	Mean
Your CPEs	    2.8	    4.1	    5.3	    8.9	    10.9
Baseline CPEs	14.7	40.1	46.4	65.9	94.5
Speedup		    5.3	    9.9	    8.8	    7.4	    8.7	    7.8

Rotate: Version = rotate: Current working version:
Dim		        64	    128	    256	    512	    1024	Mean
Your CPEs	    2.3	    2.3	    2.4	    2.4	    4.8
Baseline CPEs	14.7	40.1	46.4	65.9	94.5
Speedup		    6.4	    17.1	19.0	28.0	19.6	16.3
```

### 1.3 矩阵分块
矩阵分块也可以认为是循环展开的一种，我们以32为矩阵分块的单位，代码如下：
``` c
void rotate(int dim, pixel *src, pixel *dst) 
{
	int i, j, k, l;
    int div = 32;
    for (i = 0; i < dim; i += div) {
        for (j = 0; j < dim; j += div) {
            for (k = i; k < i + div; k++) {
                for (l = j; l < j + div; l++) {
                    dst[RIDX(k, l, dim)] = src[RIDX(l, dim - 1 - k, dim)];
                }
            }
        }
    }
}
```
执行结果如下，可以发现，这种方式和前一种有着差不多的优化速度，且代码可读性更佳。
``` bash
$ ./driver
Rotate: Version = naive_rotate: Naive baseline implementation:
Dim		        64	    128		256		512		1024	Mean
Your CPEs		2.7		3.9		5.6		8.9		10.9
Baseline CPEs	14.7	40.1	46.4	65.9	94.5
Speedup			5.5		10.4	8.3		7.4		8.6		7.9

Rotate: Version = rotate: Current working version:
Dim				64		128		256		512		1024	Mean
Your CPEs		2.1		2.1		2.5		2.6		5.3
Baseline CPEs	14.7	40.1	46.4	65.9	94.5
Speedup			7.0		19.5	18.9	25.1	17.9	16.3
```

## 2 Smooth
这题采用分类讨论和减少函数调用的方式去做，如下代码所示：
``` c
void smooth(int dim, pixel *src, pixel *dst) 
{
	// Smooth the four corners.
    int curr;

    // Left Top Corner
    dst[0].red   = (src[0].red + src[1].red + src[dim].red + src[dim + 1].red) >> 2;
    dst[0].green = (src[0].green + src[1].green + src[dim].green + src[dim + 1].green) >> 2;
    dst[0].blue  = (src[0].blue + src[1].blue + src[dim].blue + src[dim + 1].blue) >> 2;

    // Right Top Corner
    curr = dim - 1;
    dst[curr].red   = (src[curr].red + src[curr - 1].red + src[curr + dim - 1].red + src[curr + dim].red) >> 2;
    dst[curr].green = (src[curr].green + src[curr - 1].green + src[curr + dim - 1].green + src[curr + dim].green) >> 2;
    dst[curr].blue  = (src[curr].blue + src[curr - 1].blue + src[curr + dim - 1].blue + src[curr + dim].blue) >> 2;

    // Left Bottom Corner
    curr *= dim;
    dst[curr].red   = (src[curr].red + src[curr + 1].red + src[curr - dim].red + src[curr - dim + 1].red) >> 2;
    dst[curr].green = (src[curr].green + src[curr + 1].green + src[curr - dim].green + src[curr - dim + 1].green) >> 2;
    dst[curr].blue  = (src[curr].blue + src[curr + 1].blue + src[curr - dim].blue + src[curr - dim + 1].blue) >> 2;

    // Right Bottom Corner
    curr += dim - 1;
    dst[curr].red   = (src[curr].red + src[curr - 1].red + src[curr - dim].red + src[curr - dim - 1].red) >> 2;
    dst[curr].green = (src[curr].green + src[curr - 1].green + src[curr - dim].green + src[curr - dim - 1].green) >> 2;
    dst[curr].blue  = (src[curr].blue + src[curr - 1].blue + src[curr - dim].blue + src[curr - dim - 1].blue) >> 2;

    // Smooth four edges
    int ii, jj, limit;

    // Top edge 
    limit = dim - 1;
    for (ii = 1; ii < limit; ii++)
    {
        dst[ii].red   = (src[ii].red + src[ii - 1].red + src[ii + 1].red + src[ii + dim].red + src[ii + dim - 1].red + src[ii + dim + 1].red) / 6;
        dst[ii].green = (src[ii].green + src[ii - 1].green + src[ii + 1].green + src[ii + dim].green + src[ii + dim - 1].green + src[ii + dim + 1].green) / 6;
        dst[ii].blue  = (src[ii].blue + src[ii - 1].blue + src[ii + 1].blue + src[ii + dim].blue + src[ii + dim - 1].blue + src[ii + dim + 1].blue) / 6;
    }

    // Bottom Edge 
    limit = dim * dim - 1;
    for (ii = (dim - 1) * dim + 1; ii < limit; ii++)
    {
        dst[ii].red   = (src[ii].red + src[ii - 1].red + src[ii + 1].red + src[ii - dim].red + src[ii - dim - 1].red + src[ii - dim + 1].red) / 6;
        dst[ii].green = (src[ii].green + src[ii - 1].green + src[ii + 1].green + src[ii - dim].green + src[ii - dim - 1].green + src[ii - dim + 1].green) / 6;
        dst[ii].blue  = (src[ii].blue + src[ii - 1].blue + src[ii + 1].blue + src[ii - dim].blue + src[ii - dim - 1].blue + src[ii - dim + 1].blue) / 6;
    }

    // Left edge
    limit = dim * (dim - 1);
    for (jj = dim; jj < limit; jj += dim)
    {
        dst[jj].red = (src[jj].red + src[jj + 1].red + src[jj - dim].red + src[jj - dim + 1].red + src[jj + dim].red + src[jj + dim + 1].red) / 6;
        dst[jj].green = (src[jj].green + src[jj + 1].green + src[jj - dim].green+ src[jj - dim + 1].green + src[jj + dim].green + src[jj + dim + 1].green) / 6;
        dst[jj].blue = (src[jj].blue + src[jj + 1].blue + src[jj - dim].blue + src[jj - dim + 1].blue + src[jj + dim].blue + src[jj + dim + 1].blue) / 6;
    }

    // Right Edge
    for (jj = 2 * dim - 1 ; jj < limit ; jj += dim)
    {
        dst[jj].red = (src[jj].red + src[jj - 1].red + src[jj - dim].red + src[jj - dim - 1].red + src[jj + dim].red + src[jj + dim - 1].red) / 6;
        dst[jj].green = (src[jj].green + src[jj - 1].green + src[jj - dim].green + src[jj - dim - 1].green + src[jj + dim].green + src[jj + dim - 1].green) / 6;
        dst[jj].blue = (src[jj].blue + src[jj - 1].blue + src[jj - dim].blue + src[jj - dim - 1].blue + src[jj + dim].blue + src[jj + dim - 1].blue) / 6;
    }

    // Remaining pixels
    int i, j;
    for (i = 1 ; i < dim - 1 ; i++) {
        for (j = 1 ; j < dim - 1 ; j++) {
            curr = i * dim + j;
            dst[curr].red = (src[curr].red + src[curr - 1].red + src[curr + 1].red + src[curr - dim].red + src[curr - dim - 1].red + src[curr - dim + 1].red + src[curr + dim].red + src[curr + dim - 1].red + src[curr + dim + 1].red) / 9;
            dst[curr].green = (src[curr].green + src[curr - 1].green + src[curr + 1].green + src[curr - dim].green + src[curr - dim - 1].green + src[curr - dim + 1].green + src[curr + dim].green + src[curr + dim - 1].green + src[curr + dim + 1].green) / 9;
            dst[curr].blue = (src[curr].blue + src[curr - 1].blue + src[curr + 1].blue + src[curr - dim].blue + src[curr - dim - 1].blue + src[curr - dim + 1].blue + src[curr + dim].blue + src[curr + dim - 1].blue + src[curr + dim + 1].blue) / 9;
        }
    }	
}
```
运行结果如下：
``` bash
$ ./driver
Smooth: Version = smooth: Current working version:
Dim				32		64		128		256		512		Mean
Your CPEs		17.8	17.6	18.6	19.2	20.1
Baseline CPEs	695.0	698.0	702.0	717.0	722.0
Speedup			39.1	39.6	37.8	37.3	36.0	37.9

Smooth: Version = naive_smooth: Naive baseline implementation:
Dim				32		64		128		256		512		Mean
Your CPEs		57.7	58.1	54.7	55.1	59.9
Baseline CPEs	695.0	698.0	702.0	717.0	722.0
Speedup			12.0	12.0	12.8	13.0	12.1	12.4
```

## 小结
书本上的知识还是很难应用到实际中，而且许多函数修改完以后可读性变差，所以这里做这个实验的感觉并没有前几个实验做得那么爽快。
