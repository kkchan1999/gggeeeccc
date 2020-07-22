#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* new_thread(void* arg)
{
    char* str = arg;
    while (1) {
        printf("hello,%s\n", str);
        sleep(1);
    }

    //退出线程
    //pthread_exit();

    return "exit";
}

int main(void)
{
    //分离属性、优先级属性、栈属性

    //线程id
    pthread_t tid;
    int retval;
    void* retpoint;

    //创建一条线程
    retval = pthread_create(&tid, NULL, new_thread, "kangkang");

    if (retval != 0) {
        fprintf(stderr, "创建线程失败\n");
        return -1;
    }
    while (1) {
        sleep(1);
        printf("创建成功！\n");
    }

    pthread_join(tid, &retpoint); //等待回收指定的线程

    return 0;
}