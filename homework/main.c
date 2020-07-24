#include "pool.h"

void mytask(staff_info_t* staff_list, task_t* tasklist)
{
    staff_info_t* ptr;
    printf("%s下单了", tasklist->info->name);

    for (ptr = staff_list->next; ptr != NULL; ptr = ptr->next) {
        if (pthread_self() == ptr->tid) {
            printf("跑腿小哥:%s\n工号:%ld\n已接单\n", ptr->name, ptr->tid);
            break;
        }
    }
    sleep(tasklist->info->time);
}

//封装一个发送任务的函数
void send_task(thread_pool_t* pool)
{
    task_t* task = malloc(sizeof(task_t));
    task_info_t* taskinfo = malloc(sizeof(task_info_t));
    task->info = taskinfo;

    char buf[256];

    task->task = mytask;
    task->arg = pool;

    //输入数据
    printf("请输入客户名字：");
    scanf("%s", buf);
    strcpy(task->info->name, buf);

    printf("请输入用户的电话：");
    scanf("%s", buf);
    strcpy(task->info->phone, buf);

    printf("请输入任务说明：");
    scanf("%s", buf);
    strcpy(task->info->task_text, buf);

    printf("请输入任务时长：");
    scanf("%s", buf);
    task->info->time = atoi(buf);

    printf("请输入任务佣金：");
    scanf("%s", buf);
    task->info->money = atoi(buf);

    add_task(pool, task);
}

bool staff_rest(thread_pool_t* pool)
{
    if (pool->staff_info_list->next == NULL) {
        printf("没人上班！\n");
        return false;
    }

    printf("有以下员工在干活：\n");
    for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        printf("%s ", ptr->name);
    }

    printf("\n请输入谁要休息：");
    char buf[64];
    scanf("%s", buf);

    staff_info_t* ptr = NULL;
    staff_info_t* prev = NULL;
    for (ptr = pool->staff_info_list->next, prev = pool->staff_info_list; ptr != NULL; prev = ptr, ptr = ptr->next) {
        if (strcmp(buf, ptr->name) == 0) {
            pthread_mutex_lock(&pool->lock);
            prev->next = ptr->next; //修改链表
            pthread_mutex_unlock(&pool->lock);
            pthread_cancel(ptr->tid); //取消线程
            pthread_join(ptr->tid, NULL); //接合线程

            printf("%ld号员工:%s已经休息了，他一共赚了%d元\n", ptr->tid, ptr->name, ptr->money);

            pool->active_threads--;

            free(ptr); //释放资源
            return true;
        }
    }

    printf("输入有误，请重试\n");
    return false;
}

bool input_staff(thread_pool_t* pool)
{
    staff_info_t* ptr = malloc(sizeof(staff_info_t));

    char buf[256];

    printf("请输入员工姓名：");
    scanf("%s", buf);
    strcpy(ptr->name, buf);

    printf("请输入员工电话：");
    scanf("%s", buf);
    strcpy(ptr->phone, buf);

    printf("请输入员工性别（0或1）：");
    scanf("%s", buf);
    int sex = atoi(buf);
    if (sex != 0 && sex != 1) {
        printf("输入错误！\n");
    } else {
        ptr->sex = sex;
    }

    ptr->money = 0; //新员工还想有钱？
    ptr->next = NULL;

    add_staff(pool, ptr);

    return true;
}

int main(int argc, char const* argv[])
{
    //初始化
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    init_pool(pool, 3);

    char buf[4]; //用来放选项
    bool exit_flag = false;
    while (!exit_flag) {
        scanf("%s", buf);
        switch (atoi(buf)) {
        case 1:
            printf("任务发送\n");
            send_task(pool);
            break;

        case 2:
            printf("员工注册\n");
            input_staff(pool);
            break;

        case 3:
            printf("员工休息\n");
            staff_rest(pool);
            break;

        case 4:
            exit_flag = true;
            break;

        case 5:
            for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
                printf("%ld:%s有%d元了\n", ptr->tid, ptr->name, ptr->money);
            }

            break;

        default:
            printf("输入错误,重新输入\n");
            break;
        }
    }

    free(pool);

    return 0;
}
