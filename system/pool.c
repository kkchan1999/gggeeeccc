#include "pool.h"

void hander(void* arg)
{
    pthread_mutex_unlock((pthread_mutex_t*)arg); //防止死锁
}

void* routine(void* arg)
{
    printf("爷开始干活了,爷的tid是:%ld\n", pthread_self());
    thread_pool_t* pool = (thread_pool_t*)arg; //将线程池的地址存其来
    struct task* p; //整个指针用来遍历任务列表

    while (1) {
        pthread_cleanup_push(hander, (void*)&pool->lock);
        pthread_mutex_lock(&pool->lock);

        while (pool->watting_tasks == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        if (pool->watting_tasks == 0 && pool->shutdown == true) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }

        p = pool->task_list->next;
        pool->task_list->next = p->next;
        pool->watting_tasks--;
        pthread_mutex_unlock(&pool->lock);
        pthread_cleanup_pop(0);

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        (p->task)(pool, p->arg);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        free(p);
    }
    pthread_exit(NULL);
}
/*---------------------------------------------- 
 *
 *
 *----------------------------------------------*/
bool init_pool(thread_pool_t* pool, unsigned int thread_number)
{
    pthread_mutex_init(&pool->lock, NULL); //初始化互斥锁
    pthread_cond_init(&pool->cond, NULL); //初始化条件变量

    pool->shutdown = false;
    pool->task_list = malloc(sizeof(struct task)); //申请任务链表头
    pool->task_info_list = malloc(sizeof(task_info_t)); //申请任务信息链表头
    pool->staff_info_list = malloc(sizeof(staff_info_t)); //申请员工链表头

    if (pool->staff_info_list == NULL || pool->task_info_list == NULL || pool->task_list == NULL) {
        perror("申请内存失败");
        return false;
    }

    pool->task_list->next = NULL;
    pool->task_info_list = NULL;
    pool->watting_tasks = 0;
    pool->active_threads = thread_number;

    //创建员工出了点问题
    //员工是个链表,所以用链表的方法去处理
    staff_info_t* p = NULL;
    for (int i = 0; i < pool->active_threads; i++) {
        p = calloc(1, sizeof(staff_info_t)); //申请内存
        p->money = 0;
        p->sex = true;
        strcpy(p->name, "admin"); //随便了
        strcpy(p->phone, "123456"); //电话随便弄个吧
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
}

bool add_task(thread_pool_t* pool, void*(task)(staff_info_t*, task_info_t*), void* arg)
{
    struct task* new_task = malloc(sizeof(struct task));
    if (new_task == NULL) {
        perror("申请内存失败!");
        return false;
    }

    new_task->task = task;
    new_task->arg = arg;
    new_task->next = NULL;
    // new_task
}

int main(int argc, char const* argv[])
{
    thread_pool_t pool;
    if (init_pool(&pool, 5)) {
        printf("成功!\n");
    }

    for (staff_info_t* temp = pool.staff_info_list; temp != NULL; temp = temp->next) {
        printf("%ld,%s\n", temp->tid, temp->name);
    }

    printf("haha\n");
    sleep(5);
    return 0;
}
