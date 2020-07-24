#include "pool.h"

void hander(void* arg)
{
    pthread_mutex_unlock((pthread_mutex_t*)arg); //防止死锁
}

void* routine(void* arg)
{
    pthread_t my_tid = pthread_self();
    printf("爷开始干活了,爷的tid是:%ld\n", my_tid);
    thread_pool_t* pool = (thread_pool_t*)arg; //将线程池的地址存起来
    struct task* p; //整个指针用来遍历任务列表

    //把自己的信息拿过来
    //首先在链表里面找到自己

    staff_info_t* my_info
        = pool->staff_info_list;
    while (my_info != NULL) {
        if (my_info->tid == my_tid) {
            printf("找到员工信息了!:%ld\t%s\t%s\t%d\t%d\n",
                my_info->tid,
                my_info->name,
                my_info->phone,
                my_info->sex,
                my_info->money);
            break;
        } else {
            my_info = my_info->next;
        }
    }
    if (my_info == NULL) {
        printf("信息找不到,肯定有地方出错了!\n");
        pthread_exit(NULL);
    }

    //开始循环
    while (1) {
        pthread_cleanup_push(hander, (void*)&pool->lock);
        pthread_mutex_lock(&pool->lock);

        while (pool->watting_tasks == 0 && !pool->shutdown) {
            printf("%ld没事干,睡觉\n", pthread_self());
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        //被叫醒,看看是不是真的有任务
        if (pool->watting_tasks == 0 && pool->shutdown == true) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }

        //这里开始应该是确认开始任务的,后期可以在这里修改

        //把任务拿走然后解锁
        p = pool->task_list->next;
        pool->task_list->next = p->next;
        pool->watting_tasks--;

        pthread_mutex_unlock(&pool->lock);
        pthread_cleanup_pop(0);

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        (p->task)(pool->staff_info_list, p);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        //执行完之后收钱阿
        my_info->money += p->info->money;

        //释放之前申请的内存
        free(p->info);
        free(p);
    }
    pthread_exit(NULL);
}

bool init_pool(thread_pool_t* pool, unsigned int thread_number)
{
    pthread_mutex_init(&pool->lock, NULL); //初始化互斥锁
    pthread_cond_init(&pool->cond, NULL); //初始化条件变量

    pool->shutdown = false;
    pool->task_list = malloc(sizeof(struct task)); //申请任务链表头
    pool->staff_info_list = malloc(sizeof(staff_info_t)); //申请员工链表头

    if (pool->staff_info_list == NULL || pool->task_list == NULL) {
        perror("申请内存失败");
        return false;
    }

    pool->task_list->next = NULL;
    //pool->task_info_list = NULL;
    pool->watting_tasks = 0;
    pool->active_threads = thread_number;

    //创建员工出了点问题
    //员工是个链表,所以用链表的方法去处理
    for (int i = 0; i < pool->active_threads; i++) {
        staff_info_t* p = NULL;
        p = malloc(sizeof(staff_info_t)); //申请内存
        p->money = 0;
        p->sex = true;
        strcpy(p->name, "admin"); //随便了
        strcpy(p->phone, "123456"); //随便弄个吧
        p->next = NULL;

        if (pthread_create(&(p->tid), NULL, routine, (void*)pool) != 0) {
            perror("创建线程失败!");
            return false;
        }
        staff_info_t* temp = pool->staff_info_list;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = p;
    }

    return true;
}

//任务投放
void add_task(thread_pool_t* pool, task_t* task)
{
    pthread_mutex_lock(&pool->lock);

    task->next = pool->task_list->next;
    pool->task_list->next = task;

    pthread_mutex_unlock(&pool->lock);

    pool->watting_tasks++;

    //叫醒一个线程
    pthread_cond_signal(&pool->cond);
}

//添加线程
bool add_staff(thread_pool_t* pool, staff_info_t* staff)
{
    if (pool->active_threads >= MAX_ACTIVE_THREADS) {
        printf("员工太多了！\n");
        return false;
    }

    staff_info_t* temp = pool->staff_info_list;
    while (temp != NULL) {
        temp = temp->next;
    }
    temp->next = staff; //把新线程插在最后

    pool->active_threads++; //活动线程数+1
    return true;
}
