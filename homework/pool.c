#include "pool.h"

void hander(void* arg)
{
    pthread_mutex_unlock((pthread_mutex_t*)arg); //防止死锁
}

void* routine(void* arg)
{
    pthread_t my_tid = pthread_self();

    thread_pool_t* pool = (thread_pool_t*)arg; //将线程池的地址存起来
    struct task *p, *prev; //整个指针用来遍历任务列表

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
    printf("%s:开始上班\n", my_info->name);

    //开始循环
    while (1) {
        pthread_cleanup_push(hander, (void*)&pool->lock);
        pthread_mutex_lock(&pool->lock);

        while (pool->watting_tasks == 0 && !pool->shutdown) {
            printf("%s没事干,睡觉\n", my_info->name);
            pthread_cond_wait(&pool->cond, &pool->lock);
        }

        //被叫醒,看看是不是真的有任务
        if (pool->watting_tasks == 0 && pool->shutdown == true) {
            pthread_mutex_unlock(&pool->lock);
            pthread_exit(NULL);
        }

        //这里开始应该是确认开始任务的,后期可以在这里修改

        //弄个flag看看有没有vip
        bool vip_flag = false;
        //遍历任务列表里面有没有vip任务
        for (prev = pool->task_list, p = pool->task_list->next; p != NULL; prev = p, p = p->next) {
            if (p->info->vip) { //vip的情况
                vip_flag = true;
                prev->next = p->next;
                pool->watting_tasks--;
                break;
            }
        }

        //没有vip的情况
        if (!vip_flag) {
            p = pool->task_list->next;
            pool->task_list->next = p->next;
            pool->watting_tasks--;
        }

        pthread_mutex_unlock(&pool->lock);
        pthread_cleanup_pop(0);

        print_task(p->info);

        time_t t;
        struct tm* local_tm;
        time(&t);
        local_tm = localtime(&t);
        time_t t1 = mktime(local_tm);

        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        (p->task)(pool->staff_info_list, p); //开始任务
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

        time(&t);
        local_tm = localtime(&t);
        time_t t2 = mktime(local_tm);
        time_t finsh_time = t2 - t1; //最终用时

        //执行完之后收钱,收钱原则:规定时间内做完收用户给的,超时多少就收超时部分*2
        int salary = p->info->money;
        printf("%s完成%s的任务,用时%lds,", my_info->name, p->info->name, finsh_time);
        if (finsh_time > p->info->time) {
            printf("超时%ld秒,", finsh_time - p->info->time);
            salary = p->info->money + (finsh_time - p->info->time) * 2;
        }
        printf("此次任务薪水为:%d元,", salary);
        my_info->money += salary;
        printf("%s目前有%d元了\n", my_info->name, my_info->money);

        //释放之前创建任务申请的内存
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

        p->onwork = false;

        sem_init(&p->sem, 0, 0); //初始化信号量

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
    task_t* ptr = pool->task_list;
    while (ptr->next != NULL) {
        ptr = ptr->next; //指向链表的最后
    }

    pthread_cleanup_push(hander, (void*)&pool->lock);
    pthread_mutex_lock(&pool->lock);
    //用尾插才对!!头插没办法做到先来后到!

    ptr->next = task;

    //task->next = pool->task_list->next;
    //pool->task_list->next = task;

    pthread_mutex_unlock(&pool->lock);
    pthread_cleanup_pop(0);

    pool->watting_tasks++;

    //叫醒一个线程
    pthread_cond_signal(&pool->cond);
}

//添加线程
bool add_staff(thread_pool_t* pool, staff_info_t* staff)
{
    staff_info_t* temp = pool->staff_info_list;
    while (temp->next != NULL) {
        temp = temp->next;
    }

    temp->next = staff; //把新线程的信息插在最后

    if (pthread_create(&staff->tid, NULL, routine, pool) != 0) {
        perror("创建线程失败");
        return false;
    }

    pool->active_threads++; //活动线程数+1
    return true;
}

//关闭前会把所有任务处理完
bool destroy_pool(thread_pool_t* pool)
{
    // if (pool->watting_tasks != 0) {
    //     printf("还没完成所有任务,不能退出!\n");
    //     return false;
    // }

    //自动分配完成所有任务,妥善退出.
    for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        for (int i = 0; i < pool->active_threads + pool->watting_tasks; i++) { //线程数+等待任务数,有冗余
            sem_post(&ptr->sem);
        }
    }
    printf("sem_post success\n");

    pool->shutdown = true; //使能退出开关
    pthread_cond_broadcast(&pool->cond); //唤醒全部线程

    //接合线程
    for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        errno = pthread_join(ptr->tid, NULL);
        if (errno != 0) {
            printf("接合 线程:%ld 失败,errno:%s\n", ptr->tid, strerror(errno));
        } else {
            print_staff(ptr);
            printf("%ld接合成功\n", ptr->tid);
            pool->active_threads--;
        }
    }
    if (pool->active_threads == 0) {
        printf("全部线程接合成功!\n");
    }

    //释放staff的资源
    if (pool->staff_info_list->next == NULL) {
        free(pool->staff_info_list); //防止没有任务的时候不释放这块内存
    } else {
        check_money(pool);
        staff_info_t* delete_ptr = pool->staff_info_list;
        staff_info_t* ptr = pool->staff_info_list->next;
        for (; ptr != NULL; ptr = delete_ptr = ptr, ptr = ptr->next) {
            free(delete_ptr);
        }
    }

    free(pool->task_list);
    return true;
}

bool print_staff(staff_info_t* staff)
{
    FILE* f = fopen("./staff.txt", "a+");
    if (f == NULL) {
        perror("打开staff文件失败!");
        return false;
    }

    fprintf(f, "%s#%s#%d#%d#\n", staff->name, staff->phone, staff->sex, staff->money);

    fclose(f);

    return true;
}

bool print_task(task_info_t* task)
{
    FILE* f = fopen("./task.txt", "a+");
    if (f == NULL) {
        perror("打开task文件失败");
        return false;
    }

    fprintf(f, "%s#%s#%s#%d#%d#%d#\n", task->name, task->phone, task->task_text, task->vip, task->time, task->money);

    fclose(f);
    return true;
}

bool read_staff(FILE* f, staff_info_t* staff)
{
    char buf[256];

    if (fgets(buf, 256, f) == NULL) {
        return false;
    }

    char name[256];
    char phone[256];
    char sex[4];
    char money[16];
    bzero(money, 16);
    bzero(name, 256);
    bzero(phone, 256);
    bzero(sex, 4);

    char c;

    int i;
    for (i = 0; c != '#'; i++, c = buf[i]) {
        name[i] = buf[i];
    }

    c = 's'; //改成不是'#'就行,后面还要用来判断的
    i = i + 1; //往后移位
    for (int j = 0; c != '#'; i++, j++, c = buf[i]) {
        phone[j] = buf[i];
    }

    c = 's';
    i = i + 1;
    for (int j = 0; c != '#'; i++, j++, c = buf[i]) {
        sex[j] = buf[i];
    }
    c = 's';
    i = i + 1;
    for (int j = 0; c != '#'; i++, j++, c = buf[i]) {
        money[j] = buf[i];
    }

    strcpy(staff->name, name);
    strcpy(staff->phone, phone);

    staff->money = atoi(money);
    staff->sex = atoi(sex);
    return true;
}
