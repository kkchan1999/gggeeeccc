//编写程序，通过键盘输入，当输入create创建一条线程，并且将所有的线程的ID及线程函数名及参数做成一个结构体节点，形成一条链表，当键盘输入exit时，最先创建的线退出程
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct node {
    pthread_t id;
    char* name; //暂时这么搞吧
    struct node* next;
} listnode;

void thread_func(void* arg)
{
    pthread_t str = *(pthread_t*)arg;
    printf("%ld:爷启动了！\n", str);
    for (int i = 0; i < 20; i++) {
        usleep(500);
        printf("%ld:爷又来了\n", str);
    }
    printf("111\n");
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid;
    pthread_create(&tid, NULL, (void*)&thread_func, &tid);
    //char* p = calloc(1, 128);
    usleep(500000);
    pthread_join(tid, NULL);
    printf("over\n");
    //printf("%s\n", p);
    //free(p);
}