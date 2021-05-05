## 实验要求
该实验已经完成了大部分的框架，我们只需要按照需求在`tsh.c`文件中实现以下的函数功能：
* `eval`：解析和解释命令行的函数，大约70行；
* `builtin_cmd`：识别内置命令的函数，内置命令包括：`quit`、`fg`、`bg`和`job`，大约25行；
* `do_bgfg`：实现`fg`和`bg`的内置命令的杉树，大概50行；
* `waitfg`：等待前台作业完成函数，大概20行；
* `sigchld_handler`：SIGCHILD信号处理函数；
* `sigtstp_handler`：SIGTSTP (ctrl-z)信号处理函数；
* `sigint_handler`：SIGINT (ctrl-c)信号处理函数；

具体的要求可以参考writeup。

## eval函数
``` c
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    int bg;
    pid_t pid;
    sigset_t mask_all, mask_one, prev_one;

    bg = parseline(cmdline, argv);
    if (argv[0] == NULL) return;
    if (!builtin_cmd(argv)) {
        sigfillset(&mask_all);
        sigemptyset(&mask_one);
        sigaddset(&mask_one, SIGCHLD);
        
        sigprocmask(SIG_BLOCK, &mask_one, &prev_one);
        if((pid = fork()) == 0) {
            setpgid(0, 0); // 创建一个进程组，其进程组id就是其pid
            sigprocmask(SIG_SETMASK, &prev_one, NULL);
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: command not found\n", argv[0]);
                exit(0);
            }
        }

        // 对全局数据结构jobs进行访问时，要阻塞所有信号
        sigprocmask(SIG_BLOCK, &mask_all, &prev_one);
        addjob(jobs, pid, bg ? BG : FG, cmdline);
        sigprocmask(SIG_SETMASK, &prev_one, NULL);

        if(bg) printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        else waitfg(pid);
    }
}
```
在eval函数中，我们首先通过parseline函数解析输入的命令行，然后根据builtin_cmd函数判断是否是内置命令，如果是内置命令那直接在builtin_cmd函数中就执行了。

如果不是内置命令，则需要在shell中新建一个子进程来执行该命令，这个时候需要《深入理解计算机系统 第三版》P525的eval函数和P543用sigprocmask来同步进程。

最后，如果该命令是前台程序（fg），那么shell需要等待该程序结束；如果是后台程序，那么shell打印后继续读取新的command。

## builtin_cmd函数
``` c
int builtin_cmd(char **argv) 
{
   
    if (!strcmp(argv[0], "quit")) exit(0);
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    if (!strcmp(argv[0], "bg") || !strcmp(argv[0],"fg")) {
        do_bgfg(argv);
        return 1;
    }
    return 0;     /* not a builtin command */
}
```
这个直接参考书本P525的builtin_command函数。

## do_bgfg函数
``` c
void do_bgfg(char **argv) 
{
    int jid;
    pid_t pid;
    struct job_t *job;
    sigset_t mask, prev;

    if (argv[1] == NULL) {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    if(sscanf(argv[1], "%%%d", &jid) > 0) {
        job = getjobjid(jobs, jid);
        if (job == NULL || job->state == UNDEF) {
            printf("%s: No such job\n", argv[1]);
			return;
        }
    } else if (sscanf(argv[1], "%d", &pid) > 0) {
        job = getjobpid(jobs, pid); 
        if (job == NULL || job->state == UNDEF) {
            printf("%s: No such process\n", argv[1]);
			return;
        }
    } else {
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
		return;
    }

    sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, &prev);
	if(!strcmp(argv[0], "fg")) job->state = FG;
	else job->state = BG;
	sigprocmask(SIG_SETMASK, &prev, NULL);

    kill(-job->pid, SIGCONT);
    if(!strcmp(argv[0], "fg")) waitfg(job->pid);
    else printf("[%d] (%d) %s", job->jid, pid, job->cmdline);
}
```
这个函数主要用来执行`bg <job>`和`fg <job>`内置命令。不管输入的是pid还是jid，我们取出job，然后设置状态，最后发送SIGCONT给整个`|pid|`进程组的进程去继续进程，如果是后台进程，直接打印就好了，然后继续工作；如果是前台进程，shell需要等程序执行完。

## waitfg函数
这里我们采取sigsuspend的方式进行等待，当子程序结束，内核会给shell发送一个SIGCHILD信号，这时会唤醒shell。
``` c
void waitfg(pid_t pid)
{
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    sigaddset(&wait_mask, SIGUSR1);
    while(pid == fgpid(jobs))
        if(sigsuspend(&wait_mask) != -1)
            unix_error("sigsuspend error");
}
```

## sigint_handler和sigtstp_handler
``` c
void sigint_handler(int sig) 
{
    int old_errno = errno;	//首先需要保存原始的errno
    pid_t pid = fgpid(jobs);
    if(pid != 0){
        kill(-pid, sig);
    }
	errno = old_errno; 
}
```

``` c
void sigtstp_handler(int sig) 
{
    int old_errno = errno;	//首先需要保存原始的errno
    pid_t pid = fgpid(jobs);
    if(pid != 0){
        kill(-pid, sig);
    }
	errno = old_errno; 
}
```
这两个函数比较简单，都是直接像子进程发送信号。

## sigchld_handler函数
``` c
void sigchld_handler(int sig) 
{
    int old_errno = errno;	//首先需要保存原始的errno
	pid_t pid;
	sigset_t mask, prev;
	int state;	//保存waitpid的状态，用来判断子进程是终止还是停止
	struct job_t *job;
	
	sigfillset(&mask);
	//由于信号不存在队列，而waitpid一次只会回收一个子进程，所以用whild
	while((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0){	//要检查停止和终止的，并且不要卡在这个循环中
		//对全局结构变量jobs进行修改时，要阻塞所有信号
		sigprocmask(SIG_BLOCK, &mask, &prev);
		if(WIFEXITED(state)){	//子进程通过调用exit或return正常终止，需要从jobs中删除该作业
			deletejob(jobs, pid);
		}else if(WIFSIGNALED(state)){	//子进程因为一个未捕获的信号终止
			printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(state));		
			deletejob(jobs, pid);
		}else if(WIFSTOPPED(state)){	//如果子进程是停止的，需要修改改作业的状态
			job = getjobpid(jobs, pid);
			job->state = ST;
			printf("Job [%d] (%d) stopped by signal %d\n", job->jid, pid, WSTOPSIG(state));
		}
		sigprocmask(SIG_SETMASK, &prev, NULL);	//恢复信号接收
	}
	errno = old_errno;
}
```
本函数需参考P539的handler2函数，尽可能多的处理信号。

## 验证
对比了代码的执行情况，如下，可以发现二者基本一致。
``` bash
$ make test15
./sdriver.pl -t trace15.txt -s ./tsh -a "-p"
#
# trace15.txt - Putting it all together
#
tsh> ./bogus
./bogus: command not found
tsh> ./myspin 10
Job [1] (5337) terminated by signal 2
tsh> ./myspin 3 &
[1] (5339) ./myspin 3 &
tsh> ./myspin 4 &
[2] (5341) ./myspin 4 &
tsh> jobs
[1] (5339) Running ./myspin 3 &
[2] (5341) Running ./myspin 4 &
tsh> fg %1
Job [1] (5339) stopped by signal 20
tsh> jobs
[1] (5339) Stopped ./myspin 3 &
[2] (5341) Running ./myspin 4 &
tsh> bg %3
%3: No such job
tsh> bg %1
[1] (0) ./myspin 3 &
tsh> jobs
[1] (5339) Running ./myspin 3 &
[2] (5341) Running ./myspin 4 &
tsh> fg %1
tsh> quit
```
```bash
$ make rtest15
./sdriver.pl -t trace15.txt -s ./tshref -a "-p"
#
# trace15.txt - Putting it all together
#
tsh> ./bogus
./bogus: Command not found
tsh> ./myspin 10
Job [1] (5287) terminated by signal 2
tsh> ./myspin 3 &
[1] (5289) ./myspin 3 &
tsh> ./myspin 4 &
[2] (5291) ./myspin 4 &
tsh> jobs
[1] (5289) Running ./myspin 3 &
[2] (5291) Running ./myspin 4 &
tsh> fg %1
Job [1] (5289) stopped by signal 20
tsh> jobs
[1] (5289) Stopped ./myspin 3 &
[2] (5291) Running ./myspin 4 &
tsh> bg %3
%3: No such job
tsh> bg %1
[1] (5289) ./myspin 3 &
tsh> jobs
[1] (5289) Running ./myspin 3 &
[2] (5291) Running ./myspin 4 &
tsh> fg %1
tsh> quit
```
