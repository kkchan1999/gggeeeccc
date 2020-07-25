#ifndef _POOL_H_
#define _POOL_H_

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

//#define MAX_WAITING_TASKS 1000 //最大的等待任务数量
//#define MAX_ACTIVE_THREADS 20 //最大的线程数量

//任务信息结构体
typedef struct task_info {
    char name[64]; //姓名
    char phone[16]; //电话号码

    char task_text[256]; //任务说明

    int money; //佣金数量
    int time; //花费的时间

    //struct task_info* next; //下一个任务信息节点
} task_info_t;

//任务链表结构体
typedef struct task {
    void (*task)(void* staff_list, void* userinfo); //函数指针
    void* arg; //传入函数的参数
    task_info_t* info;
    struct task* next; //下一个任务节点
} task_t;

//员工信息结构体
typedef struct staff_info {
    pthread_t tid;
    char name[256];
    char phone[256];
    bool sex;
    int money; //记录一下赚了多少钱
    bool onwork;
    sem_t sem; //信号量,用来手动停止
    struct staff_info* next;
} staff_info_t;

typedef struct thread_pool {
    pthread_mutex_t lock; //线程池同步用的互斥锁
    pthread_cond_t cond; //条件变量,用于线程睡眠

    task_t* task_list; //任务链表
    //task_info_t* task_info_list; //任务信息链表
    staff_info_t* staff_info_list; //员工信息链表,里面包含了tid

    unsigned watting_tasks; //等待的任务数量,也是任务链表的长度
    unsigned active_threads; //目前已经创建的线程数(员工数量)

    bool shutdown; //线程池的开关

} thread_pool_t;

//遍历链表
#define list_for_each(list_head, ptr) \
    for (ptr = list_head->next; ptr != NULL; ptr = ptr->NULL)

void hander(void* arg);
void* routine(void* arg);
bool init_pool(thread_pool_t* pool, unsigned int thread_number);
void add_task(thread_pool_t* pool, task_t* task);
bool add_staff(thread_pool_t* pool, staff_info_t* staff);
bool destroy_pool(thread_pool_t* pool);

void check_money(thread_pool_t* pool);

#endif