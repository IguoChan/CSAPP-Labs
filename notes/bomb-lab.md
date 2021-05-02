## 前期准备
首先我们反汇编可执行文件`bomb`，得到汇编代码。
``` bash
$ objdump -d bomb > disassemble.asm
```

## phase_1
首先我们定位到main函数，在264行，这里可以对比以下C代码和汇编代码，可以发现都是一一对应的。然后直接找到phase_1，如下所示：
``` x86asm
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	retq
```
寄存器%esi是默认的第二个参数储存寄存器，直接存储了值0x402400，然后调用函数strings_not_equal，我们可以跳到strings_not_equal函数，可以发现这个函数应该是一个比较两个string是否相等的函数（其实可以仔细看看这个函数的实现，但是这里不详述）。

然后我们就可以gdb调试了，如下：
``` bash
$ gdb bomb
(gdb) b phase_1							# 在phase_1函数打断点
Breakpoint 1 at 0x400ee0
(gdb) r 								# 运行
Starting program: /home/topeet/smb_share/workspace/CSAPP/CSAPP-Labs/bomb-lab/bomb 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
hello									#输入hello

Breakpoint 1, 0x0000000000400ee0 in phase_1 ()
(gdb) layout asm 						# 方便查看代码
(gdb) stepi 							# 单步执行，这里执行两次，到0x400ee9这一行
(gdb) i registers rdi rsi 				# 查看寄存器rdi，rsi的值
rdi            0x603780 6305664
rsi            0x402400 4203520
(gdb) x /s 0x603780						# 查看存储在rdi中的地址上的值，发现正是我输入的字符串“hello”
0x603780 <input_strings>:        "hello"
(gdb) x /s 0x402400						# 查看存储在rsi中的地址上的值，发现如下的字符串
0x402400:        "Border relations with Canada have never been better."
```
根据以上分析可以发现，程序将输入的字符串存储进默认的第一个参数储存寄存器%rdi，将固定地址0x402400的字符串存储到默认的第二个参数储存寄存器%rsi（%esi取其低32位），然后调用strings_not_equal函数对比这两个参数，那么我们可以大胆猜想，希望我们输入的即为字符串`Border relations with Canada have never been better.`，如下：
``` bash
$ ./bomb 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
```
可以发现，phase_1还是比较简单的。

## phase_2
和前面一题一样，进去以后发现调用了read_six_numbers函数，字面意思就是读取6个数，需要注意的是，这时候如第四行所示`sub $0x28,%rsp`，在栈内留下了0x28大小的空间，且`mov %rsp,%rsi`将此时的栈地址赋值给了%rsi寄存器。
``` x86asm
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp
  400f02:	48 89 e6             	mov    %rsp,%rsi
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>
  ...
```

然后我们进入read_six_numbers函数，上来第2行就预留了0x18大小的空间，这是为6个int大小的整数准备的。
``` x86asm
000000000040145c <read_six_numbers>:
  40145c:	48 83 ec 18          	sub    $0x18,%rsp
  401460:	48 89 f2             	mov    %rsi,%rdx
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
  401470:	48 8d 46 10          	lea    0x10(%rsi),%rax
  401474:	48 89 04 24          	mov    %rax,(%rsp)
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
  401485:	b8 00 00 00 00       	mov    $0x0,%eax
  40148a:	e8 61 f7 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  40148f:	83 f8 05             	cmp    $0x5,%eax
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d>
  401494:	e8 a1 ff ff ff       	callq  40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	retq
```
后面的操作看起来有些奇怪，其实理解了sscanf函数的形式就理解了，sscanf函数的在这里有8个输入参数，第一个参数是input；第二个参数是模式串，如下，在执行到sscanf函数前查看%rsi：
``` bash
(gdb) i registers rsi
rsi            0x4025c3 4203971
(gdb) x /s 0x4025c3
0x4025c3:        "%d %d %d %d %d %d"
```
第三到第八个参数是读取后6个值的存储地址，分别为%rdx,%rcx,%r8,%r9.(%rsp),8(%rsp)处，对应着0x00(%rsi),0x4(%rsi),0x8(%rsi),0xc(%rsi),0x10(%rsi),0x14(%rsi),0x18(%rsi)，最重要的是，我们要记住，在执行`mov    $0x4025c3,%esi`之前，%rsi存储的值是phase_2刚进来执行完`sub $0x28,%rsp`时的栈帧值，故而相当于将sscanf的结果输出到phase_2预留的栈帧中。

从read_six_numbers返回到phase_2函数后，根据以上分析，在下列第7行时，%rsp存储的地址上存储的就是输入6个数中的第一个数，根据代码是和1相比较，如果不等就bomb，所以第一个数一定是1。
``` x86asm
0000000000400efc <phase_2>:
  400efc: 55                    push   %rbp
  400efd: 53                    push   %rbx
  400efe: 48 83 ec 28           sub    $0x28,%rsp
  400f02: 48 89 e6              mov    %rsp,%rsi
  400f05: e8 52 05 00 00        callq  40145c <read_six_numbers>
  400f0a: 83 3c 24 01           cmpl   $0x1,(%rsp)
  400f0e: 74 20                 je     400f30 <phase_2+0x34>
  400f10: e8 25 05 00 00        callq  40143a <explode_bomb>
  400f15: eb 19                 jmp    400f30 <phase_2+0x34>
  400f17: 8b 43 fc              mov    -0x4(%rbx),%eax
  400f1a: 01 c0                 add    %eax,%eax
  400f1c: 39 03                 cmp    %eax,(%rbx)
  400f1e: 74 05                 je     400f25 <phase_2+0x29>
  400f20: e8 15 05 00 00        callq  40143a <explode_bomb>
  400f25: 48 83 c3 04           add    $0x4,%rbx
  400f29: 48 39 eb              cmp    %rbp,%rbx
  400f2c: 75 e9                 jne    400f17 <phase_2+0x1b>
  400f2e: eb 0c                 jmp    400f3c <phase_2+0x40>
  400f30: 48 8d 5c 24 04        lea    0x4(%rsp),%rbx
  400f35: 48 8d 6c 24 18        lea    0x18(%rsp),%rbp
  400f3a: eb db                 jmp    400f17 <phase_2+0x1b>
  400f3c: 48 83 c4 28           add    $0x28,%rsp
  400f40: 5b                    pop    %rbx
  400f41: 5d                    pop    %rbp
  400f42: c3                    retq 
```
如果第一个数是1，则跳往0x400f30地址，这里将接下来的下一个地址赋值给%rbx，将最大的地址赋值给%rbp，然后跳往0x400f17。从0x400f17开始的代码含义是，拿后一个数和前数的两倍进行比较，相等就跳入0x400f25，不相等就爆炸；而0x400f25处的代码是和最大的地址进行比较，这类似于于for循环的计数。所以很明显可以看出，输出的就是以1开头，2为公比，一共6个的等比数列，即{1,2,4,8,16,32}，如下：
``` bash
$ ./bomb 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
```

## phase_3
我们继续查看phase_3的汇编代码，可以看到调用sscanf函数来读取参数。
``` x86asm
0000000000400f43 <phase_3>:
  400f43: 48 83 ec 18           sub    $0x18,%rsp
  400f47: 48 8d 4c 24 0c        lea    0xc(%rsp),%rcx
  400f4c: 48 8d 54 24 08        lea    0x8(%rsp),%rdx
  400f51: be cf 25 40 00        mov    $0x4025cf,%esi
  400f56: b8 00 00 00 00        mov    $0x0,%eax
  400f5b: e8 90 fc ff ff        callq  400bf0 <__isoc99_sscanf@plt>
  ...
```
然后第二个参数地址是0x4025cf，在gdb调试中查看，可以发现是读取两个整数。
``` bash
(gdb) x /s 0x4025cf
0x4025cf:        "%d %d"
```
继续查看，发现返回值是和1进行比较，需要大于1，而sscanf的返回值表示读取的个数，我觉得这是不严谨的地方，其实输入两个以上的数字也不会报错。
``` x86asm
  ...
  400f60: 83 f8 01              cmp    $0x1,%eax
  400f63: 7f 05                 jg     400f6a <phase_3+0x27>
  400f65: e8 d0 04 00 00        callq  40143a <explode_bomb>
  ...
```
接下来对读取的第一个数，也就是输入的第一个数和7比较，只要0-7都是可以的。然后间接跳转到一个地址，这个地址是多少取决于输入的第一个数。
``` x86asm
  ...
  400f6a: 83 7c 24 08 07        cmpl   $0x7,0x8(%rsp)
  400f6f: 77 3c                 ja     400fad <phase_3+0x6a>
  400f71: 8b 44 24 08           mov    0x8(%rsp),%eax
  400f75: ff 24 c5 70 24 40 00  jmpq   *0x402470(,%rax,8)
  ...
```
我们看看这个数随着我们输入的数的变化。
``` bash
(gdb) x 0x402470              # 输入的第一个数为0
0x402470:       0x00400f7c
(gdb) x 0x402478              # 输入的第一个数为1
0x402478:       0x00400fb9
(gdb) x 0x402480              # 输入的第一个数为2
0x402480:       0x00400f83
(gdb) x 0x402488              # 输入的第一个数为3
0x402488:       0x00400f8a
(gdb) x 0x402490              # 输入的第一个数为4
0x402490:       0x00400f91
(gdb) x 0x402498              # 输入的第一个数为5
0x402498:       0x00400f98
(gdb) x 0x4024a0              # 输入的第一个数为6
0x4024a0:       0x00400f9f
(gdb) x 0x4024a8              # 输入的第一个数为7
0x4024a8:       0x00400fa6
```
发现跳往这些步骤的操作都是一样，即将第二个数与某一个固定的数对比，相等才能通过，譬如当第一个数为0时，第二个数必须为0xcf，也就是207，总结下来如下表：

|输入第一个数|输入的第二个数|
|:--|:--|
|0|207|
|1|311|
|2|707|
|3|256|
|4|389|
|5|206|
|6|682|
|7|327|

以输入第一个参数等于0为例，如下：
``` bash
$ ./bomb 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207
Halfway there!
```

## phase_4
我们发现phase_4的前面几行和phase_3没有区别，说明也是读入两个整数，第一个参数需要比14小；然后通过输入func4中进行比对，需输出为0，且func4有三个入参，其中第一个参数`x`即为我们输入的第一个参数，第二个参数`y`为0，第三个参数`z`为14（0xe），func4应该是比较复杂的函数，还用到了递归，但是我们可以走最简单的路线，也就是当满足`x=((z-y)+((z-y)>>31))/2+y`时，带入参数，即`x=7`时，func4输出为0，这种情况最为简单。
``` x86asm
0000000000400fce <func4>:
  400fce: 48 83 ec 08           sub    $0x8,%rsp
  400fd2: 89 d0                 mov    %edx,%eax
  400fd4: 29 f0                 sub    %esi,%eax
  400fd6: 89 c1                 mov    %eax,%ecx
  400fd8: c1 e9 1f              shr    $0x1f,%ecx
  400fdb: 01 c8                 add    %ecx,%eax
  400fdd: d1 f8                 sar    %eax
  400fdf: 8d 0c 30              lea    (%rax,%rsi,1),%ecx
  400fe2: 39 f9                 cmp    %edi,%ecx
  400fe4: 7e 0c                 jle    400ff2 <func4+0x24>
  400fe6: 8d 51 ff              lea    -0x1(%rcx),%edx
  400fe9: e8 e0 ff ff ff        callq  400fce <func4>
  400fee: 01 c0                 add    %eax,%eax
  400ff0: eb 15                 jmp    401007 <func4+0x39>
  400ff2: b8 00 00 00 00        mov    $0x0,%eax
  400ff7: 39 f9                 cmp    %edi,%ecx
  400ff9: 7d 0c                 jge    401007 <func4+0x39>
  400ffb: 8d 71 01              lea    0x1(%rcx),%esi
  400ffe: e8 cb ff ff ff        callq  400fce <func4>
  401003: 8d 44 00 01           lea    0x1(%rax,%rax,1),%eax
  401007: 48 83 c4 08           add    $0x8,%rsp
  40100b: c3                    retq
```
第二个参数需要和0相等，取0即可，所以可以输入：7 0，如下（其它情况就不列举了）：
``` bash
$ ./bomb 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207
Halfway there!
7 0
So you got that one.  Try this one.
```

## phase_5
首先进入phase_5会进行一系列的常规操作，然后调用了string_length函数，结果和6比较，不相等则bomb，所以应当输入一个长度为6的字符串。然后直接跳转到4010d2地址，这里只是将eax寄存器置零后再跳转到40108b位置。

``` x86asm
0000000000401062 <phase_5>:
  401062: 53                    push   %rbx
  401063: 48 83 ec 20           sub    $0x20,%rsp
  401067: 48 89 fb              mov    %rdi,%rbx
  40106a: 64 48 8b 04 25 28 00  mov    %fs:0x28,%rax
  401071: 00 00 
  401073: 48 89 44 24 18        mov    %rax,0x18(%rsp)
  401078: 31 c0                 xor    %eax,%eax
  40107a: e8 9c 02 00 00        callq  40131b <string_length>
  40107f: 83 f8 06              cmp    $0x6,%eax
  401082: 74 4e                 je     4010d2 <phase_5+0x70>
  401084: e8 b1 03 00 00        callq  40143a <explode_bomb>
  401089: eb 47                 jmp    4010d2 <phase_5+0x70>
  40108b: 0f b6 0c 03           movzbl (%rbx,%rax,1),%ecx
  40108f: 88 0c 24              mov    %cl,(%rsp)
  401092: 48 8b 14 24           mov    (%rsp),%rdx
  401096: 83 e2 0f              and    $0xf,%edx
  401099: 0f b6 92 b0 24 40 00  movzbl 0x4024b0(%rdx),%edx
  4010a0: 88 54 04 10           mov    %dl,0x10(%rsp,%rax,1)
  4010a4: 48 83 c0 01           add    $0x1,%rax
  4010a8: 48 83 f8 06           cmp    $0x6,%rax
  4010ac: 75 dd                 jne    40108b <phase_5+0x29>
  4010ae: c6 44 24 16 00        movb   $0x0,0x16(%rsp)
  4010b3: be 5e 24 40 00        mov    $0x40245e,%esi
  4010b8: 48 8d 7c 24 10        lea    0x10(%rsp),%rdi
  4010bd: e8 76 02 00 00        callq  401338 <strings_not_equal>
  4010c2: 85 c0                 test   %eax,%eax
  4010c4: 74 13                 je     4010d9 <phase_5+0x77>
  4010c6: e8 6f 03 00 00        callq  40143a <explode_bomb>
  4010cb: 0f 1f 44 00 00        nopl   0x0(%rax,%rax,1)
  4010d0: eb 07                 jmp    4010d9 <phase_5+0x77>
  4010d2: b8 00 00 00 00        mov    $0x0,%eax
  4010d7: eb b2                 jmp    40108b <phase_5+0x29>
  4010d9: 48 8b 44 24 18        mov    0x18(%rsp),%rax
  4010de: 64 48 33 04 25 28 00  xor    %fs:0x28,%rax
  4010e5: 00 00 
  4010e7: 74 05                 je     4010ee <phase_5+0x8c>
  4010e9: e8 42 fa ff ff        callq  400b30 <__stack_chk_fail@plt>
  4010ee: 48 83 c4 20           add    $0x20,%rsp
  4010f2: 5b                    pop    %rbx
  4010f3: c3                    retq
```
接下来就进入到正题了，可以发现，401067行将输入的第一个参数传递给了rbx，我们可以用指令在gdb上查看，发现正是输入字符串的地址。再回到40108b行，这一行将从rbx开始的第一个byte输入到ecx中，其实也就是我们输入的第一个参数，然后将这个数进行了传来传去传到rdx中，最后和0xf进行与操作，并将结果存入rdx。然后将地址0x4024b0(%rdx)的那个byte存入edx，并最后存入0x10(%rsp,%rax,1)（注意此时rax=0），即此地址从0x10(%rsp)开始，后续会持续6此，每次持续中将rax加一，结合前面的操作，可知是将输入的字符串中6个字符依次取出进行以上操作作为索引，从地址0x4024b0(%rdx)依次取出字符组成的字符串和存储在0x40245e处的字符串进行对比，使之相等即能通过测试，而两处的字符串分别为：
``` bash
(gdb) x /s 0x4024b0
0x4024b0 <array.3449>:   "maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
(gdb) x /s 0x40245e
0x40245e:        "flyers"
```
分析一下可知，和0xf与操作只能得到0-15的索引，而字符串"maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"前十六个里面（"maduiersnfotvbyl"）包含了"flyers"的每个字符，所以可以取成功。譬如，'f'在索引9处，我们可以取字符'I'(0x49)；'l'在索引15处，我们可以取字符'O'(0x4f)；'y'在索引14处，我们可以取字符'N'(0x4e)；'e'在索引5处，我们可以取字符'E'(0x45)；'r'在索引6处，我们可以取字符'F'(0x46)；'s'在索引7处，我们可以取字符'G'(0x47)。最后答案如下：
``` bash
$ ./bomb 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Border relations with Canada have never been better.
Phase 1 defused. How about the next one?
1 2 4 8 16 32
That's number 2.  Keep going!
0 207 
Halfway there!
7 0
So you got that one.  Try this one.
IONEFG
Good work!  On to the next...
```
当然，这题还有很多满足条件的答案。

## phase_6
phase_6的代码如下：
``` x86asm
00000000004010f4 <phase_6>:
  4010f4: 41 56                 push   %r14
  4010f6: 41 55                 push   %r13
  4010f8: 41 54                 push   %r12
  4010fa: 55                    push   %rbp
  4010fb: 53                    push   %rbx
  4010fc: 48 83 ec 50           sub    $0x50,%rsp
  401100: 49 89 e5              mov    %rsp,%r13
  401103: 48 89 e6              mov    %rsp,%rsi
  401106: e8 51 03 00 00        callq  40145c <read_six_numbers>
  40110b: 49 89 e6              mov    %rsp,%r14
  40110e: 41 bc 00 00 00 00     mov    $0x0,%r12d
  401114: 4c 89 ed              mov    %r13,%rbp
  401117: 41 8b 45 00           mov    0x0(%r13),%eax
  40111b: 83 e8 01              sub    $0x1,%eax
  40111e: 83 f8 05              cmp    $0x5,%eax
  401121: 76 05                 jbe    401128 <phase_6+0x34>
  401123: e8 12 03 00 00        callq  40143a <explode_bomb>
  401128: 41 83 c4 01           add    $0x1,%r12d
  40112c: 41 83 fc 06           cmp    $0x6,%r12d
  401130: 74 21                 je     401153 <phase_6+0x5f>
  401132: 44 89 e3              mov    %r12d,%ebx
  401135: 48 63 c3              movslq %ebx,%rax
  401138: 8b 04 84              mov    (%rsp,%rax,4),%eax
  40113b: 39 45 00              cmp    %eax,0x0(%rbp)
  40113e: 75 05                 jne    401145 <phase_6+0x51>
  401140: e8 f5 02 00 00        callq  40143a <explode_bomb>
  401145: 83 c3 01              add    $0x1,%ebx
  401148: 83 fb 05              cmp    $0x5,%ebx
  40114b: 7e e8                 jle    401135 <phase_6+0x41>
  40114d: 49 83 c5 04           add    $0x4,%r13
  401151: eb c1                 jmp    401114 <phase_6+0x20>
  401153: 48 8d 74 24 18        lea    0x18(%rsp),%rsi
  401158: 4c 89 f0              mov    %r14,%rax
  40115b: b9 07 00 00 00        mov    $0x7,%ecx
  401160: 89 ca                 mov    %ecx,%edx
  401162: 2b 10                 sub    (%rax),%edx
  401164: 89 10                 mov    %edx,(%rax)
  401166: 48 83 c0 04           add    $0x4,%rax
  40116a: 48 39 f0              cmp    %rsi,%rax
  40116d: 75 f1                 jne    401160 <phase_6+0x6c>
  40116f: be 00 00 00 00        mov    $0x0,%esi
  401174: eb 21                 jmp    401197 <phase_6+0xa3>
  401176: 48 8b 52 08           mov    0x8(%rdx),%rdx
  40117a: 83 c0 01              add    $0x1,%eax
  40117d: 39 c8                 cmp    %ecx,%eax
  40117f: 75 f5                 jne    401176 <phase_6+0x82>
  401181: eb 05                 jmp    401188 <phase_6+0x94>
  401183: ba d0 32 60 00        mov    $0x6032d0,%edx
  401188: 48 89 54 74 20        mov    %rdx,0x20(%rsp,%rsi,2)
  40118d: 48 83 c6 04           add    $0x4,%rsi
  401191: 48 83 fe 18           cmp    $0x18,%rsi
  401195: 74 14                 je     4011ab <phase_6+0xb7>
  401197: 8b 0c 34              mov    (%rsp,%rsi,1),%ecx
  40119a: 83 f9 01              cmp    $0x1,%ecx
  40119d: 7e e4                 jle    401183 <phase_6+0x8f>
  40119f: b8 01 00 00 00        mov    $0x1,%eax
  4011a4: ba d0 32 60 00        mov    $0x6032d0,%edx
  4011a9: eb cb                 jmp    401176 <phase_6+0x82>
  4011ab: 48 8b 5c 24 20        mov    0x20(%rsp),%rbx
  4011b0: 48 8d 44 24 28        lea    0x28(%rsp),%rax
  4011b5: 48 8d 74 24 50        lea    0x50(%rsp),%rsi
  4011ba: 48 89 d9              mov    %rbx,%rcx
  4011bd: 48 8b 10              mov    (%rax),%rdx
  4011c0: 48 89 51 08           mov    %rdx,0x8(%rcx)
  4011c4: 48 83 c0 08           add    $0x8,%rax
  4011c8: 48 39 f0              cmp    %rsi,%rax
  4011cb: 74 05                 je     4011d2 <phase_6+0xde>
  4011cd: 48 89 d1              mov    %rdx,%rcx
  4011d0: eb eb                 jmp    4011bd <phase_6+0xc9>
  4011d2: 48 c7 42 08 00 00 00  movq   $0x0,0x8(%rdx)
  4011d9: 00 
  4011da: bd 05 00 00 00        mov    $0x5,%ebp
  4011df: 48 8b 43 08           mov    0x8(%rbx),%rax
  4011e3: 8b 00                 mov    (%rax),%eax
  4011e5: 39 03                 cmp    %eax,(%rbx)
  4011e7: 7d 05                 jge    4011ee <phase_6+0xfa>
  4011e9: e8 4c 02 00 00        callq  40143a <explode_bomb>
  4011ee: 48 8b 5b 08           mov    0x8(%rbx),%rbx
  4011f2: 83 ed 01              sub    $0x1,%ebp
  4011f5: 75 e8                 jne    4011df <phase_6+0xeb>
  4011f7: 48 83 c4 50           add    $0x50,%rsp
  4011fb: 5b                    pop    %rbx
  4011fc: 5d                    pop    %rbp
  4011fd: 41 5c                 pop    %r12
  4011ff: 41 5d                 pop    %r13
  401201: 41 5e                 pop    %r14
  401203: c3                    retq
```
代码很长，分析如下：



首先保护现场，压栈操作；然后依次读取了6个数，可以记作a0-a5；大循环，要求ai<=6，包含小循环，要求任意的ai!=aj (i>j)；然后操作ai=7-ai；最后会发现一个地址，这个地址是一个链表，几经查找才打印出以下信息：
``` bash
p /x *0x6032d0@24
$2 = {0x14c, 0x1, 0x6032e0, 0x0, 0xa8, 0x2, 0x6032f0, 0x0, 0x39c, 0x3, 0x603300, 0x0, 0x2b3, 0x4, 0x603310, 0x0, 0x1dd, 0x5, 0x603320, 0x0, 0x1bb, 0x6, 0x0, 0x0}
```

可以分析出，这个链表有6个元素，分别是{val, index, next, 0, val, index, next, 0, ...}这样的排列。

最后一段逻辑是将第a0, a1, …, a4个节点的next设置为第a1, a2, …, a5个节点，第a5个节点的next为空，要求链表每个节点的值要大于他的next节点的值，否则爆炸。

根据以上打印的链表值，从大到小应该是3 4 5 6 1 2的顺序排列，而经过前面ai=7-ai的变化，所以应该是4 3 2 1 6 5。

我们将以上答案写进一个名为anwser的文件中，如下：
``` bash
Border relations with Canada have never been better.
1 2 4 8 16 32
0 207
7 0
YONEFG
4 3 2 1 6 5
```

运行如下：
``` bash
$ ./bomb answer 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Phase 1 defused. How about the next one?
That's number 2.  Keep going!
Halfway there!
So you got that one.  Try this one.
Good work!  On to the next...
Congratulations! You've defused the bomb!
```

## secret_phase
secret_phase函数不是在main函数里调用的，而是在phase_defused函数里调用的，这个不查看反汇编代码很难察觉，是个隐藏关。
phase_defused的代码如下：
``` x86asm
00000000004015c4 <phase_defused>:
  4015c4: 48 83 ec 78           sub    $0x78,%rsp
  4015c8: 64 48 8b 04 25 28 00  mov    %fs:0x28,%rax
  4015cf: 00 00 
  4015d1: 48 89 44 24 68        mov    %rax,0x68(%rsp)
  4015d6: 31 c0                 xor    %eax,%eax
  4015d8: 83 3d 81 21 20 00 06  cmpl   $0x6,0x202181(%rip)        # 603760 <num_input_strings>
  4015df: 75 5e                 jne    40163f <phase_defused+0x7b>
  4015e1: 4c 8d 44 24 10        lea    0x10(%rsp),%r8
  4015e6: 48 8d 4c 24 0c        lea    0xc(%rsp),%rcx
  4015eb: 48 8d 54 24 08        lea    0x8(%rsp),%rdx
  4015f0: be 19 26 40 00        mov    $0x402619,%esi
  4015f5: bf 70 38 60 00        mov    $0x603870,%edi
  4015fa: e8 f1 f5 ff ff        callq  400bf0 <__isoc99_sscanf@plt>
  4015ff: 83 f8 03              cmp    $0x3,%eax
  401602: 75 31                 jne    401635 <phase_defused+0x71>
  401604: be 22 26 40 00        mov    $0x402622,%esi
  401609: 48 8d 7c 24 10        lea    0x10(%rsp),%rdi
  40160e: e8 25 fd ff ff        callq  401338 <strings_not_equal>
  401613: 85 c0                 test   %eax,%eax
  401615: 75 1e                 jne    401635 <phase_defused+0x71>
  401617: bf f8 24 40 00        mov    $0x4024f8,%edi
  40161c: e8 ef f4 ff ff        callq  400b10 <puts@plt>
  401621: bf 20 25 40 00        mov    $0x402520,%edi
  401626: e8 e5 f4 ff ff        callq  400b10 <puts@plt>
  40162b: b8 00 00 00 00        mov    $0x0,%eax
  401630: e8 0d fc ff ff        callq  401242 <secret_phase>
  401635: bf 58 25 40 00        mov    $0x402558,%edi
  40163a: e8 d1 f4 ff ff        callq  400b10 <puts@plt>
  40163f: 48 8b 44 24 68        mov    0x68(%rsp),%rax
  401644: 64 48 33 04 25 28 00  xor    %fs:0x28,%rax
  40164b: 00 00 
  40164d: 74 05                 je     401654 <phase_defused+0x90>
  40164f: e8 dc f4 ff ff        callq  400b30 <__stack_chk_fail@plt>
  401654: 48 83 c4 78           add    $0x78,%rsp
  401658: c3                    retq   
  401659: 90                    nop
  40165a: 90                    nop
  40165b: 90                    nop
  40165c: 90                    nop
  40165d: 90                    nop
  40165e: 90                    nop
  40165f: 90                    nop
```
我们注意4015d8行，这里把常数6和一个固定地址603760（即常量num_input_strings的地址）进行对比，当不等时直接退出函数，相等时继续执行4015e1后的代码。后面的代码其实挺好理解的，即调用sscanf函数，我们查看以下两个地址：
``` bash
(gdb) x /s 0x402619
0x402619:        "%d %d %s"
(gdb) x /s 0x603870
0x603870 <input_strings+240>:    "7 0"
```
第一个正是读入两个整数一个字符串，第二个地址正式phase_4中的输入（说明我们仅输入两个数无法触发secret_phase），然后会将这个输入分别装到0x8(%rsp)，0xc(%rsp)和0x10(%rsp)中，再拿0x10(%rsp)中的字符串和0x402622地址的字符串对比，如果相等才会触发secret_phase。
``` bash
(gdb) x /s 0x402622
0x402622:        "DrEvil"
```
所以我们在phase_4的输入中外加字符串"DrEvil"即可触发secret_phase，其函数如下：
``` x86asm
0000000000401242 <secret_phase>:
  401242: 53                    push   %rbx
  401243: e8 56 02 00 00        callq  40149e <read_line>
  401248: ba 0a 00 00 00        mov    $0xa,%edx
  40124d: be 00 00 00 00        mov    $0x0,%esi
  401252: 48 89 c7              mov    %rax,%rdi
  401255: e8 76 f9 ff ff        callq  400bd0 <strtol@plt>
  40125a: 48 89 c3              mov    %rax,%rbx
  40125d: 8d 40 ff              lea    -0x1(%rax),%eax
  401260: 3d e8 03 00 00        cmp    $0x3e8,%eax
  401265: 76 05                 jbe    40126c <secret_phase+0x2a>
  401267: e8 ce 01 00 00        callq  40143a <explode_bomb>
  40126c: 89 de                 mov    %ebx,%esi
  40126e: bf f0 30 60 00        mov    $0x6030f0,%edi
  401273: e8 8c ff ff ff        callq  401204 <fun7>
  401278: 83 f8 02              cmp    $0x2,%eax
  40127b: 74 05                 je     401282 <secret_phase+0x40>
  40127d: e8 b8 01 00 00        callq  40143a <explode_bomb>
  401282: bf 38 24 40 00        mov    $0x402438,%edi
  401287: e8 84 f8 ff ff        callq  400b10 <puts@plt>
  40128c: e8 33 03 00 00        callq  4015c4 <phase_defused>
  401291: 5b                    pop    %rbx
  401292: c3                    retq
```
总结起来就是输入一个数，通过strtol函数转化为long型（不允许超过0x3e8），然后将这个数作为第二个参数输入fun7，第一个参数是立即数0x6030f0，我们继续进入fun7。
``` x86asm
0000000000401204 <fun7>:
  401204: 48 83 ec 08           sub    $0x8,%rsp
  401208: 48 85 ff              test   %rdi,%rdi
  40120b: 74 2b                 je     401238 <fun7+0x34>
  40120d: 8b 17                 mov    (%rdi),%edx
  40120f: 39 f2                 cmp    %esi,%edx
  401211: 7e 0d                 jle    401220 <fun7+0x1c>
  401213: 48 8b 7f 08           mov    0x8(%rdi),%rdi
  401217: e8 e8 ff ff ff        callq  401204 <fun7>
  40121c: 01 c0                 add    %eax,%eax
  40121e: eb 1d                 jmp    40123d <fun7+0x39>
  401220: b8 00 00 00 00        mov    $0x0,%eax
  401225: 39 f2                 cmp    %esi,%edx
  401227: 74 14                 je     40123d <fun7+0x39>
  401229: 48 8b 7f 10           mov    0x10(%rdi),%rdi
  40122d: e8 d2 ff ff ff        callq  401204 <fun7>
  401232: 8d 44 00 01           lea    0x1(%rax,%rax,1),%eax
  401236: eb 05                 jmp    40123d <fun7+0x39>
  401238: b8 ff ff ff ff        mov    $0xffffffff,%eax
  40123d: 48 83 c4 08           add    $0x8,%rsp
  401241: c3                    retq
```
以下均参考[csapp-Bomblab总结](https://zhuanlan.zhihu.com/p/272861057)。
fun7是一个递归函数，总结起来就是下面：
``` c
if ( [rdi] > esi ) 
   return 2 * fun7( [rdi + 0x8], esi );
else if ( [rdi] < esi )
   return 2 * fun7( [rdi + 0x10], esi ) + 1;
else 
   return 0;
```
我们希望fun7的返回值是2，而`2 = ((((0*2)*2)*2…)*2+1)*2`，因此第一个调用需要进入第一分支，第二次调用（第一次递归）进入第二个分支，后续如果还有，则不能进入第二分支。所以我们查看一下从0x6030f0之后的地址内容：
``` bash
(gdb) x /60a 0x6030f0
0x6030f0 <n1>:  0x24  0x603110 <n21>
0x603100 <n1+16>: 0x603130 <n22>  0x0
0x603110 <n21>: 0x8 0x603190 <n31>
0x603120 <n21+16>:  0x603150 <n32>  0x0
0x603130 <n22>: 0x32  0x603170 <n33>
0x603140 <n22+16>:  0x6031b0 <n34>  0x0
0x603150 <n32>: 0x16  0x603270 <n43>
0x603160 <n32+16>:  0x603230 <n44>  0x0
0x603170 <n33>: 0x2d  0x6031d0 <n45>
0x603180 <n33+16>:  0x603290 <n46>  0x0
0x603190 <n31>: 0x6 0x6031f0 <n41>
0x6031a0 <n31+16>:  0x603250 <n42>  0x0
0x6031b0 <n34>: 0x6b  0x603210 <n47>
0x6031c0 <n34+16>:  0x6032b0 <n48>  0x0
0x6031d0 <n45>: 0x28  0x0
0x6031e0 <n45+16>:  0x0 0x0
0x6031f0 <n41>: 0x1 0x0
0x603200 <n41+16>:  0x0 0x0
0x603210 <n47>: 0x63  0x0
0x603220 <n47+16>:  0x0 0x0
0x603230 <n44>: 0x23  0x0
0x603240 <n44+16>:  0x0 0x0
0x603250 <n42>: 0x7 0x0
0x603260 <n42+16>:  0x0 0x0
0x603270 <n43>: 0x14  0x0
0x603280 <n43+16>:  0x0 0x0
0x603290 <n46>: 0x2f  0x0
0x6032a0 <n46+16>:  0x0 0x0
0x6032b0 <n48>: 0x3e9 0x0
0x6032c0 <n48+16>:  0x0 0x0
```
从节点的关系可以看出这是一个二叉树，[rdi + 0x8]是左儿子的地址，[rdi + 0x10]是右儿子的地址，且该二叉树的深度只有4，根据以上分析，只有root->Lch->Rch和root->Lch->Rch->Lch这两种情况，即\<n32\>节点和\<n43\>节点两种情况，即输入值是0x16=22和0x14=20，这就是这题的答案。

## 答案
我们将这些答案写在answer文件中，文件和运行结果分别如下：
``` bash
Border relations with Canada have never been better.
1 2 4 8 16 32
0 207
7 0 DrEvil
YONEFG
4 3 2 1 6 5
22
```
``` bash
$ ./bomb answer 
Welcome to my fiendish little bomb. You have 6 phases with
which to blow yourself up. Have a nice day!
Phase 1 defused. How about the next one?
That's number 2.  Keep going!
Halfway there!
So you got that one.  Try this one.
Good work!  On to the next...
Curses, you've found the secret phase!
But finding it and solving it are quite different...
Wow! You've defused the secret stage!
Congratulations! You've defused the bomb!
```
