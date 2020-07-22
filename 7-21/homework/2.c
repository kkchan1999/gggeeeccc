//2，定义一全局的buffer字符串，开启两条线程，一条专门用于每隔1秒打印buffer变量中的值，一条用于监控键盘，当键盘输入数据就去改变buffer变量，输入exit则退出整个进程
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char buf[256];

pthread_mutex_t m; //互斥锁

void* output(void* arg)
{
    while (1) {
        pthread_mutex_lock(&m);
        printf("buf: %s\n", buf);
        pthread_mutex_unlock(&m);
        if (strcmp("exit", buf) == 0) {
            break;
        }
        sleep(1);
    }
    printf("output exit.\n");
    pthread_exit(NULL);
}

void* input(void* arg)
{
    char temp[256];
    while (1) {
        scanf("%s", buf);

        pthread_mutex_lock(&m);
        strcpy(temp, buf);
        pthread_mutex_unlock(&m);
        if (strcmp("exit", temp) == 0) {
            break;
        }
    }
    printf("input exit.\n");
    pthread_exit(NULL);
}

int main(void)
{
    //初始化互斥锁
    pthread_mutex_init(&m, NULL);

    pthread_t tid1, tid2;

    pthread_create(&tid1, NULL, input, NULL);
    pthread_create(&tid2, NULL, output, NULL);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    pthread_mutex_destroy(&m); //过河拆桥
    printf("搞定收工\n");

    return 0;
}
