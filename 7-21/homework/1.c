//定义一个全局变量count，创建线程1实现互斥锁的初始化，并打印一句init success，之后创建3条线程去更改count值，当count值等于0时退出所有线程，并退出整个程序
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int count = 1000;
pthread_mutex_t m; //整个互斥🔓

void* init(void* arg)
{
    //初始化互斥锁
    errno = pthread_mutex_init(&m, NULL);
    if (errno != 0) {
        perror("init failed");
        exit(1);
    }

    printf("init success\n");
    pthread_exit(NULL);
}

void* reduce(void* arg)
{
    int num = *(int*)arg; //一次减这么多
    while (count != 0) {
        pthread_mutex_lock(&m); //上锁
        if (count - num >= 0) {
            printf("%d-%d=%d\n", count, num, count - num);
            count -= num;
        }
        pthread_mutex_unlock(&m); //解锁
    }
    printf("%d要退出了\n", num);
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid1, tid2, tid3, tid4;

    pthread_create(&tid1, NULL, init, NULL); //弄个线程初始化互斥锁

    int a = 1, b = 2, c = 3;

    pthread_create(&tid2, NULL, reduce, (void*)&a);
    pthread_create(&tid3, NULL, reduce, (void*)&b);
    pthread_create(&tid4, NULL, reduce, (void*)&c);

    //接合线程
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);

    //销毁互斥锁
    pthread_mutex_destroy(&m);
    printf("搞定收工!\n");
    return 0;
}