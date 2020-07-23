#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <errno.h>
#include <pthread.h>

#define MAX_WAITING_TASKS 1000 //最大的等待任务数量
#define MAX_ACTIVE_THREADS 20 //最大的线程数量

struct task //任务链表结构体
{
    void (*task)(void* pool, void* arg); //任务做什么事情的函数指针
    void* arg; //传入函数的参数

    struct task* next; //链表的位置结构体指针
};

typedef struct info_node {
    char name[256];
    struct info_node* next;
} info_node_t;

typedef struct thread_pool {
    pthread_mutex_t lock; //用于线程池同步互斥的互斥锁
    pthread_cond_t cond; //用于让线程池里面的线程睡眠的条件变量
    struct task* task_list; //线程池的执行任务链表
    info_node_t* info_list; //添加的新的队列
    pthread_t* tids; //线程池里面线程的ID登记处

    unsigned waiting_tasks; //等待的任务数量，也就是上面任务链表的长度
    unsigned active_threads; //当前已经创建的线程数

    bool shutdown; //线程池的开关
} thread_pool;

#define list_for_each(list_head, ptr) \
    for (ptr = list_head->next; ptr != NULL; ptr = ptr->next)

bool init_pool(thread_pool* pool,
    unsigned int threads_number); //初始化线程池

bool add_task(thread_pool* pool,
    void (*task)(void*, void*),
    void* arg); //往线程池里面添加任务节点

int add_thread(thread_pool* pool,
    unsigned int additional_threads_number); //添加线程池中线程的数量

int remove_thread(thread_pool* pool,
    unsigned int removing_threads_number); //移除线程

bool destroy_pool(thread_pool* pool); //销毁线程池
void* routine(void* arg); //线程池里面线程的执行函数

bool add_info_to_pool(thread_pool* pool, char* info);
#endif
