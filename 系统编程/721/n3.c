//3用互斥锁实现，利用线程的取消机制实现每发送SIGINT信号时退出一条线程
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

pthread_t tid[10];
int count = 0;

void* thread(void* arg)
{
    while (1) {
        printf("i am thread %d\n", (int)arg);
        sleep(1);
    }
    pthread_exit(NULL);
}

void sighand(int signum)
{
    pthread_cancel(tid[count]);
    printf("我收到了%d 号信号，现在杀死了thread %d\n", signum, count);
    count++;
}
int main()
{

    signal(SIGINT, sighand);

    int thread_num[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    int i;
    for (i = 0; i < 10; i++) {
        pthread_create(&tid[i], NULL, thread, thread_num[i]);
    }

    for (i = 0; i < 10; i++)
        pthread_join(tid[i], NULL);

    printf("main exit\n");

    return 0;
}
