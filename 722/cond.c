#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

int count = 1000;

pthread_mutex_t mutex;
pthread_cond_t cond;

void *thread(void *arg)
{
	

	while(1)
	{
		pthread_mutex_lock(&mutex);//加锁操作=把锁拿过来

		printf("线程成功获得锁\n");

		while(count < 100)
		{
			pthread_cond_wait(&cond, &mutex);//解锁互斥锁，然后让线程睡眠，当被唤醒的时候再自动加上互斥锁
			continue;
		}

		count -= 100;

		printf("tid=%ld,取钱成功，还剩余=%d\n",pthread_self(),  count);

		sleep(1);

		pthread_mutex_unlock(&mutex);//解锁操作=把锁还回去
	}
	
	return 	NULL;
}

void sighand(int signum)
{
	count+=1000;
	pthread_cond_signal(&cond);//唤醒单个线程
	//pthread_cond_broadcast(&cond);//唤醒全部线程
}


int main(void)
{
	int retval;
	pthread_t tid;

	
	signal(SIGINT, sighand);

	pthread_mutex_init( &mutex, NULL);//制造一把互斥锁
	pthread_cond_init( &cond, NULL);//默认的初始化条件变量

	pthread_create(&tid, NULL, thread, &mutex);

	while(1)
	{
		pthread_mutex_lock(&mutex);//加锁操作=把锁拿过来

		printf("main成功获得锁\n");

		while(count < 100)
		{
			pthread_cond_wait(&cond, &mutex);//解锁互斥锁，然后让线程睡眠，当被唤醒的时候再自动加上互斥锁
			continue;
		}

		count -= 100;

		printf("tid=%ld,取钱成功，还剩余=%d\n",pthread_self(),  count);

		sleep(1);

		pthread_mutex_unlock(&mutex);//解锁操作=把锁还回去
	}
	


	
	pthread_join(tid, NULL);

	
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);//销毁这把互斥锁

	return 0;
}
