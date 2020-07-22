//3，利用线程的取消机制实现每发送SIGINT信号时退出一条线程
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//整个数组用来放tid吧
pthread_t tid[10];
int tid_num = 0;

void* thread(void* arg)
{
    while (1) {
        printf("%d号线程还没死\n", (int)arg);
        sleep(1);
    }
    pthread_exit(NULL);
}

void sighand(int signum)
{
    pthread_cancel(tid[tid_num]);
    printf("%d，杀死%d线程\n", signum, tid_num);
    tid_num++;
}

int main()
{

    signal(SIGINT, sighand);

    for (int i = 0; i < 10; i++) {
        pthread_create(&tid[i], NULL, thread, i);
    }

    for (int i = 0; i < 10; i++) {
        pthread_join(tid[i], NULL);
    }

    printf("搞定收工\n");

    return 0;
}