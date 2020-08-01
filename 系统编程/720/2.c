/*
2:编写一段程序，创建两个子进程，
用signal让父进程捕捉SIGINT，
当捕获到SIGINT后，父进程用kill向两个子进程分别发信号，
发送SIGUSR1给子进程1， 发送SIGUSR2给子进程2
子进程捕捉到父进程发来的信号后，分别输入下列信息后终止
Child process 1 is killed by parent!
Child process 2 is killed by parent!
父进程等待两个子进程终止后，输出以下信息后终止
Parent process exit!	
*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

pid_t cpid_1,cpid_2;

void sighand_1(int signum)		//向进程1发送信号
{
	kill(cpid_1,SIGUSR1);
}
void sighand_2(int signum)		//向进程2发送信号
{
	kill(cpid_2,SIGUSR2);
}
void printf_1(int signum)		//进程1打印
{
	printf("Child process 1 is killed by parent!\n");
	exit(1);//退出进程
}
void printf_2(int signum)		//进程2打印
{
	sleep(1);
	printf("Child process 2 is killed by parent!\n");
	exit(2);//退出进程
}
void send_signal(int signum)	//向子进程传送信号
{
	sighand_1(0);
	sighand_2(0);
}
int main(void)
{
	int status_1,status_2;
	// pid_t retpid_1,retpid_2;

	signal(SIGINT, SIG_IGN);//忽略中断信号：就算接收到中断信号也当做不存在一样
	cpid_1 = fork();//创建一个进程
	if(cpid_1 == 0)
	{
		signal(SIGUSR1, printf_1);	//进程1等待信号的到来后打印信息
		pause();
	}
	else
	{
		cpid_2 = fork();//创建一个进程
		if(cpid_2 == 0)
		{
			signal(SIGUSR2, printf_2);//进程2等待信号的到来后打印信息
			pause();
		}
		else		//父进程
		{
			while(1)
			{
				signal(SIGINT, send_signal);//父进程传送信号给子进程1
				
				if(status_1 != 1)	//判断进程1是否结束
				{
					wait(&status_1);
					status_1 = WEXITSTATUS(status_1);
				}
				if(status_2 != 2)	//判断进程2是否结束
				{
					wait(&status_2);
					status_2 = WEXITSTATUS(status_2);
				}
				if(status_1 == 1 && status_2 == 2)	//所有子进程结束
				{
					printf("Parent process exit!\n");
					exit(0);	//结束父进程
				}
			}
		}
	}
	return 0;
}


