## 基本要求和操作
我们需要了解的文件有：
* `bits.c`：我们只需在此文件中进行操作，里面有大量的注释，注释就是出题者的要求，包括你需要修改这个函数以便完成的目标、你在这个函数中所能使用的操作符种类、能使用的操作符的数目等；
* `btest.c`：运行make后会生成可执行文件，用于测试编写的函数释放正确；
* `dlc`：用于检测代码风格；
* `driver.pl`：最后用于打分；
* `fshow.c`和`ishow.c`：make后可以得到两个可执行文件，分别可以输出你输入的float数和integer数的内存表示形式，比如标志位是什么，阶码是多少等。

基本的步骤就是：
1. 修改bits.c函数注释与代码；
2. 运行`./dlc -e bits.c`查看自己用了多少操作符，以及是否有代码风格问题；
3. 编译工程，并运行`./btest`检查是否做对了；
4. 所有的做完了，最后再运行`./driver.pl`获得打分。

题目列表如下：

|名称|描述|难度|指令数目|
|:--|:--|:--|:--|
|bitXor(x,y)|只使用`~`和`&`实现|1|14|
|tmin()|返回最小补码|1|4|
|isTMax(x)|判断是否是补码最大值|1|10|
|allOddBits(x)|判断补码所有奇数位是否都是1|2|12|
|negate(x)|不使用`-`实现`-x`|2|5|
|isAsciiDigit(x)|判断`x`是否是ASCII码|3|15|
|conditional(x,y,z)|类似C语言`x?y:z`|3|16|
|isLessOrEqual(x,y)|实现`x<=y`|3|24|
|logicalNeg(x)|不使用`!`实现`!x`|4|12|
|howManyBits(x)|计算补码表达`x`所需的最少位数|4|90|
|floatScale2(uf)|计算`2.0*uf`|4|30|
|floatFloat2Int(uf)|计算`int(uf)`|4|30|
|floatPower2(x)|计算`2.0`<sup>`x`</sup>|4|30|

## bitXor
根据布尔代数，异或就是当参与运算的两个二进制数结果不同时才为1，相同时为0。可以认为，异或是同时为0或同时为1的反，即`x^y = ~((x&y)|(~(x|y)))`。题设只允许用`~`和`&`实现，而`x|y = ~(~x&~y)`，所以得出`x^y = ~(x&y)&~(~x&~y)`（注意C语言运算符优先级），如下：
```c
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
	return ~(x&y)&~(~x&~y);
}
```

## tmin
这题就比较简单了，最小的补码一定是符号位为1，其他位为0的数，所以对int(32位数)类型而言，其最小补码数就是`1<<31`。
```c
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
	return 1 << 31;
}
```

## isTmax
很明显对于补码来说，最大值就是符号位为0，其它位均为1的数。题设要求最大补码数时返回1，其它数返回0，且限定操作符。这时候需要联想到的就是，`!0 = 1`，但是其它数取反均为0，那就是说，如果我们能将最大补码数转换为0就好了。
步骤：
1. 我们假设`x`为`Tmax`，即`0111 ... 1111`，那么将`x + 1`即得到`1000 ... 0000`（注意此处不能取反得到`1000 ... 0000`），即`Tmin`；
2. 我们将`Tmin`左移1位即得到全0，但是题设不允许使用`<<`，那么我们将`Tmax + Tmin`再取反即得到0；
3. 验证其它数是否也会产生这个结果，发现如果`x = 1111 ... 1111`时，经过以上操作也会得到0，那我们需要排除这种情况，而此时`x`是个特殊的数，`!(~x) = 1`，其它任何数做这个操作都是0。
我们的代码如下：
``` c
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
	int y = x + 1;	// if x == 0x7fffffff, y = 0x80000000
	x = ~(x + y);		// x = 0
	y = !y;			// exclude if x == 0xffffffff
	x = x + y;		// exclude if x == 0xffffffff
	return !x;
}
```


## allOddBits
当整数的奇数位都为1时，返回1，否则返回0。这题其实很简单，直接用掩码就好了，当奇数位全为1时，如果把偶数位也全部填上1（与`0x55555555`的掩码取或），那么就是`0xffffffff`，这是再取反取非，即为1；如果奇数位不全为1，以上假设就不成立。代码如下：
``` c
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
	int mask = 0x55; // 不能使用超过0xff的数
	mask = (mask << 8) + mask;
	mask = (mask << 16) + mask;
	x = x | mask;
	x = ~x;
	return !x;
}
```

## negate
取负值，这个简单，就是取反加一。
代码如下：
``` c
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
	return ~x + 1;
}
```

## isAsciiDigit
这题求解整数是否在Ascii码数字0~9（即0x30~0x39）。此题还是比较有难度的，我们可以使用两个数，一个数加上比0x39大的数后符号从正变负，另一个数是加上比0x30小的数时是符号，然后再判断符号位，只要有一个符号位为1，则不在此范围内。代码如下：
``` c
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
	int Tmin = 1 << 31;
	int up = ~(Tmin | 0x39);
	int dw = ~0x2F;
	up = (up + x) >> 31;
	dw = (dw + x) >> 31;
	return !(up | dw);
}
```

## conditional
利用限定的符号实现符号`x ? y : z`的功能。逻辑如下：
1. 如果`x != 0`，那么输出y，如果`x == 0`，则输出z；
2. 如果我们能通过x的值不同转换为全0或者全1的掩码，那输出就很容易的；
代码如下：
``` c
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
	x = ~!x + 1;                // if x == 0, ~!x + 1 = -1(0xffffffff); if x != 0, ~!x + 1 = 0
	return (~x & y) | (x & z);
}
```

## isLessOrEqual
本题是利用限定的运算符实现`≤`，其实对比两个数的大小，无非就是符号相同看差值符号，符号不同正数为大。
代码如下：
``` c
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    int bitXor;
    int neg_x = ~x + 1;                     // -x
    int y_minus_x = y + neg_x;              // y-x
    int sign = y_minus_x >> 31;             // y-x的符号，无需&1，后面取的是非
    x = x >> 31;                            // x的符号
    y = y >> 31;                            // y的符号
    bitXor = (x ^ y) & 1;               // x和y符号相同标志位，相同时为0不同时为1
    return ((!bitXor)&(!sign))|(bitXor&x);  // 符号相同看差值符号，符号不同正数为大
}
```

## logicalNeg
本题是取非操作，我们知道，0的非是1，其它数的非是0。那么就是要找出0和其它数的不同，而0最大的不同就是`-0 = 0`（但是有另一个例外就是`-Tmin = Tmin`），且0的符号位和-0的符号位是相同的，均为0，而其它数的符号位和负值符号位不同（除了Tmin，Tmin的符号位和-Tmin的符号位都是1），利用这个特性，可以将0和其它数区别开来；而针对补码的右移是算数右移，即当符号位是0时，右移31位将得到0，当符号位是1时，右移31位将得到-1（全1），此时再+1，即得到0和1两个值。代码如下：
``` c
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  return ((x | (~x + 1)) >> 31) + 1;
}
```

## howManyBits
如果是一个正数，则需要找到它最高的一位（假设是n）是1的，再加上符号位，结果为n+1；如果是一个负数，则需要知道其最高的一位是0的（例如4位的1101和三位的101补码表示的是一个值：-3，最少需要3位来表示）。代码如下：
``` c
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
	int b16,b8,b4,b2,b1,b0;
	int sign=x>>31;
	x = (sign&~x)|(~sign&x);
	b16 = !!(x>>16)<<4;//高十六位是否有1
	x = x>>b16;//如果有（至少需要16位），则将原数右移16位
	b8 = !!(x>>8)<<3;//剩余位高8位是否有1
	x = x>>b8;//如果有（至少需要16+8=24位），则右移8位
	b4 = !!(x>>4)<<2;//同理
	x = x>>b4;
	b2 = !!(x>>2)<<1;
	x = x>>b2;
	b1 = !!(x>>1);
	x = x>>b1;
	b0 = x;
	return b16+b8+b4+b2+b1+b0+1;//+1表示加上符号位
}
```


## floatScale2
接下来三题都是考察对浮点数的掌握程度。
本题的意思是用int类型表征出浮点数`2*f`的形式，且输入输出都要用无符号整数表时。其实最重要的就是要掌握浮点数的表达形式：(-1)<sup>s</sup>\*M\*2<sup>E</sup>。
1. 当阶码域全0的时候（即非规格化的值），此时M = f，我们左移f，即可得到2\*uf值；
2. 当阶码域不全为0，且非全1的时候（即规格化的值），我们将阶码值加1即可得到(-1)<sup>s</sup>\*M\*2<sup>E+1</sup> = 2((-1)<sup>s</sup>\*M\*2<sup>E</sup>)；
3. 当阶码域全1时，值为无穷大或者NaN，此时直接返回uf即可，因为操作无意义。

``` c
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
    unsigned f = uf;    
    if ((f & 0x7f800000) == 0)	// 如果阶码为0，尾数左移一位即为乘以2
        f = ((f & 0x007fffff) << 1) | (0x80000000 & f);    
    else if ((f & 0x7f800000) != 0x7f800000)	// 如果阶码不为0，且非全1，则阶码加1即可
        f = f + 0x00800000;
    return f;
}
```

## floatFloat2Int
本题考查`int(uf)`，其实重点还是对浮点编码(-1)<sup>s</sup>\*M\*2<sup>E</sup>理解，具体解析的步骤在注释中。

``` c
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
    int sign = uf >> 31;                        // 得到符号
    int E = ((uf & 0x7f800000) >> 23) - 127;    // 得到E
    int M = (uf & 0x007fffff) | 0x00800000;     // 得到M 其实是真实的小数左移23，也就是下面和23做对比的原因
    if (!(uf & 0x7fffffff)) return 0;           // 如果浮点值本来就是全0，那返回0
    if (E >= 31) return 0x80000000;             // 如果真实的指数值大于等于31，则超出表达范围
    if (E < 0) return 0;                        // 如果是小数，则返回0
    if (E > 23) M = M << (E - 23);              // 如果E>23，则该数还得继续扩大2^(E-23)倍，即左移E-23位
    else M = M >> (23 - E);                     // 如果E<23，则该数还得缩小2^(23-E)倍，即右移23-E位
    if (!(M >> 31) ^ sign) return M;            // 如果M和uf符号相同，则返回M
    else return ~M + 1;                         // 如果M和uf符号不同，则返回-M
}
```

## floatPower2
这题求解的是2.0<sup>x</sup>。分析如下：
1. 首先我们要知道float类型的精度是多少，也就是最小的非规格化值是V=2<sup>-n-2<sup>k-1</sup>+2</sup>=2<sup>-149</sup>。
2. 当x<-149时，2.0<sup>x</sup>超出了浮点数的表达精度，这时返回0；
3. 当-149≤x≤-127时，这时2.0<sup>x</sup>只有小数部分（即非规格化的值），那我们只需要将1左移149+x位即可；
4. 当-127<\x<128时（即规格化的值），即将阶码编码值（x+127）左移23位即可；

代码如下：
```c
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
    int exp = 127 + x;
    int i = 149 + x;
    if (x >= -149 && x <= -127)
        return 1 << i;
    if (i < 0)
        return 0;
    if (exp >= 255)
        return 0x7f800000;
    return exp << 23;
}
```

## 结果
运行结果如下：
``` bash
$ ./btest 
Score	Rating	Errors	Function
 1	1	0	bitXor
 1	1	0	tmin
 1	1	0	isTmax
 2	2	0	allOddBits
 2	2	0	negate
 3	3	0	isAsciiDigit
 3	3	0	conditional
 3	3	0	isLessOrEqual
 4	4	0	logicalNeg
 4	4	0	howManyBits
 4	4	0	floatScale2
 4	4	0	floatFloat2Int
 4	4	0	floatPower2
Total points: 36/36

$ ./dlc -e bits.c 
dlc:bits.c:147:bitXor: 7 operators
dlc:bits.c:156:tmin: 1 operators
dlc:bits.c:176:isTmax: 7 operators
dlc:bits.c:192:allOddBits: 7 operators
dlc:bits.c:202:negate: 2 operators
dlc:bits.c:220:isAsciiDigit: 10 operators
dlc:bits.c:231:conditional: 7 operators
dlc:bits.c:248:isLessOrEqual: 13 operators
dlc:bits.c:260:logicalNeg: 5 operators
dlc:bits.c:289:howManyBits: 36 operators
dlc:bits.c:312:floatScale2: 9 operators
dlc:bits.c:336:floatFloat2Int: 20 operators
dlc:bits.c:360:floatPower2: 11 operators

$ ./driver.pl 
1. Running './dlc -z' to identify coding rules violations.

2. Compiling and running './btest -g' to determine correctness score.
gcc -O -Wall -m32 -lm -o btest bits.c btest.c decl.c tests.c 

3. Running './dlc -Z' to identify operator count violations.

4. Compiling and running './btest -g -r 2' to determine performance score.
gcc -O -Wall -m32 -lm -o btest bits.c btest.c decl.c tests.c 


5. Running './dlc -e' to get operator count of each function.

Correctness Results	Perf Results
Points	Rating	Errors	Points	Ops	Puzzle
1	1	0	2	7	bitXor
1	1	0	2	1	tmin
1	1	0	2	7	isTmax
2	2	0	2	7	allOddBits
2	2	0	2	2	negate
3	3	0	2	10	isAsciiDigit
3	3	0	2	7	conditional
3	3	0	2	13	isLessOrEqual
4	4	0	2	5	logicalNeg
4	4	0	2	36	howManyBits
4	4	0	2	9	floatScale2
4	4	0	2	20	floatFloat2Int
0	0	0	0	0	
0	0	0	0	0	10
0	4	1	2	11	floatPower2

Score = 58/62 [32/36 Corr + 26/26 Perf] (135 total operators)
```

## 小结
总体来说，data-lab不算特别的困难，只是有些地方想不到就会很难，但是一般通了以后就是一通百通了，看起来有些奇淫巧计，其实对于补码和浮点数的认知还是有很大的帮助的。
