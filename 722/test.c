/* 1，利用线程间的信号量实现一个逻辑
	  从键盘中获取数据，用另外一条线程将数据打印出来
	  如果没有打印数据出来则不能再次往里面输入字符，如果没有输入字符则不能将数据打印出来
 */

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

sem_t sem; //信号量

char buf[256];

void* input(void* arg)
{
    while (1) {
        scanf("%s", buf);
        sem_post(&sem);
        if (strcmp(buf, "exit") == 0) {
            break;
        }
    }

    printf("input exit\n");
    pthread_exit(NULL);
}

void* output(void* arg)
{
    while (1) {
        sem_wait(&sem);
        sleep(2);
        printf("buf: %s\n", buf);

        if (strcmp(buf, "exit") == 0) {
            break;
        }
    }

    printf("output exit\n");
    pthread_exit(NULL);
}

int main(int argc, char const* argv[])
{
    //初始化信号量
    sem_init(&sem, 0, 0);

    //弄两个线程
    pthread_t tid_r, tid_w;

    //创建线程
    pthread_create(&tid_r, NULL, output, NULL);
    pthread_create(&tid_w, NULL, input, NULL);

    //回收线程
    pthread_join(tid_r, NULL);
    pthread_join(tid_w, NULL);

    //销毁信号量
    sem_destroy(&sem);
    printf("搞定收工\n");
    return 0;
}
