#include "thread_pool.h"

void mytask(void* pl, void* arg)
{
    thread_pool* pool = pl;
    long n = (long)arg;

    info_node_t* ptr;

    list_for_each(pool->info_list, ptr)
    {
        printf("节点信息=%s\n", ptr->name);
    }

    printf("线程id为[%ld]的线程准备工作 %ld 秒...\n",
        pthread_self(), n);

    sleep(n);

    printf("线程id为[%ld]的线程工作 %ld 秒结束了******************\n",
        pthread_self(), n);
}

void* count_time(void* arg)
{

    int i = 0;
    while (1) {
        sleep(1);
        printf("当前是第%d秒\n", ++i);
    }

    return NULL;
}

void* get_info_name(void* arg)
{
    thread_pool* pool = arg;

    char name[256];
    while (1) {
        scanf("%s", name);

        add_info_to_pool(pool, name);
    }
}

int main(void)
{
    pthread_t a;
    pthread_create(&a, NULL, count_time, NULL); //只是用来创建一天用于计时的线程，跟线程池一点关系都没有

    // 1, initialize the pool
    thread_pool* pool = malloc(sizeof(thread_pool));
    init_pool(pool, 2);

    pthread_create(&a, NULL, get_info_name, pool);

    // 2, throw tasks
    printf("投放3个任务\n");
    add_task(pool, mytask, (void*)((rand() % 10) * 1L));
    add_task(pool, mytask, (void*)((rand() % 10) * 1L));
    add_task(pool, mytask, (void*)((rand() % 10) * 1L));

    // 3, check active threads number
    printf("当前线程数量为：%d\n",
        remove_thread(pool, 0));
    sleep(9);

    // 4, throw tasks
    printf("再次投放2个任务...\n");
    add_task(pool, mytask, (void*)((rand() % 10) * 1L));
    add_task(pool, mytask, (void*)((rand() % 10) * 1L));

    // 5, add threads
    add_thread(pool, 2);

    sleep(5);

    // 6, remove threads
    printf("删除3条线程，当前线程数还剩: %d\n",
        remove_thread(pool, 3));

    // 7, destroy the pool
    destroy_pool(pool);

    free(pool);

    return 0;
}
