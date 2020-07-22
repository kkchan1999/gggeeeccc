//头文件
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
//线程函数
void* routine(void* arg)
{
    int i, j;
    while (1) {
        fprintf(stderr, "%c ", *(char*)arg);
        //延时一段时间
        for (i = 0; i < 10000; i++)
            for (j = 0; j < 1000; j++)
                ;
    }
    pthread_exit(NULL);
}

//main函数
int main(void)
{
    //tid分配
    pthread_t tid1, tid2, tid3;
    pthread_attr_t attr1, attr2; //属性
    struct sched_param param1, param2;

    //初始化属性
    pthread_attr_init(&attr1);
    pthread_attr_init(&attr2);

    /* 设置线程是否继承创建者的调度策略 PTHREAD_EXPLICIT_SCHED：不继承才能设置线程的调度策略*/
    errno = pthread_attr_setinheritsched(&attr1, PTHREAD_EXPLICIT_SCHED);
    if (errno != 0) {
        perror("setinherit failed\n");
        return -1;
    }
    errno = pthread_attr_setinheritsched(&attr2, PTHREAD_EXPLICIT_SCHED);
    if (errno != 0) {
        perror("setinherit failed\n");
        return -1;
    }
    //SCHED_FIFO先进先出， SCHED_RR轮询， SCHED_OTHER非实时的普通线程
    errno = pthread_attr_setschedpolicy(&attr1, SCHED_RR);
    if (errno != 0) {
        perror("setschedpolicy failed\n");
        return -1;
    }
    errno = pthread_attr_setschedpolicy(&attr2, SCHED_RR);
    if (errno != 0) {
        perror("setschedpolicy failed\n");
        return -1;
    }

    //设置优先级
    param1.sched_priority
        = 1;
    param2.sched_priority = 88;

    errno = pthread_attr_setschedparam(&attr1, &param1);
    if (errno != 0) {
        perror("setschedparam failed\n");
        return -1;
    }
    errno = pthread_attr_setschedparam(&attr2, &param2);
    if (errno != 0) {
        perror("setschedparam failed\n");
        return -1;
    }

    //启动线程
    errno = pthread_create(&tid1, &attr1, routine, (void*)"1");
    if (errno != 0) {
        perror("pthread_create failed\n");
        return -1;
    }
    errno = pthread_create(&tid2, &attr2, routine, (void*)"2");
    if (errno != 0) {
        perror("pthread_create failed\n");
        return -1;
    }
    errno = pthread_create(&tid3, NULL, routine, (void*)"2");
    if (errno != 0) {
        perror("pthread_create failed\n");
        return -1;
    }

    //结合线程
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);

    //销毁属性
    pthread_attr_destroy(&attr1);
    pthread_attr_destroy(&attr2);

    return 0;
}