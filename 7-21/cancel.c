#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

pthread_t tid;

void routine(void *arg)
{
	printf("嘻嘻 arg=%s\n", (char *)arg);
}

void *thread(void *arg)
{
	int i, j, z=5;

	//pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);//设置线程不能被pthread_cancel所杀死
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);//设置线程一旦有人对线程发送取消请求则马上退出线程，不用遇到取消点函数

	pthread_cleanup_push(routine, "哈哈");//在线程被取消的时候，要求在退出线程之前执行routine里面的内容，"哈哈"则是传输给routine的参数

	while(z--)
	{
		for( i=100000; i>0; i--)
			for( j=10000; j>0; j--);

		printf("in thread\n");
	}

	pthread_cleanup_pop(1);//将pthread_cleanup_push所声明如果取消线程时执行的routine函数清除出去，0代表清楚而不执行，非0代表清楚且执行一下pthread_cleanup_push所注册的routine函数里面的内容

	z=5;
	while(z--)
	{
		for( i=100000; i>0; i--)
			for( j=10000; j>0; j--);

		printf("next thread\n");
	}

	return 	NULL;
}

void sighand(int signum)
{
	//给指定线程发送取消请求，线程遇到取消点函数才会退出
	pthread_cancel(tid);
}

int main(void)
{
	int retval;
	
	signal(SIGINT, sighand);

	pthread_create(&tid, NULL, thread, NULL);


	retval = pthread_join(tid, NULL);
	if(retval != 0)
	{
		fprintf(stderr, "join failed :%s\n", strerror(retval));
		return -1;
	}

	printf("join success\n");

	return 0;
}
