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
void send_task(thread_pool_t* pool, task_t* task)
{
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

    printf("准备投放\n");
    add_task(pool, task);
}

int main(int argc, char const* argv[])
{
    //初始化
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    init_pool(pool, 3);

    //新建任务的时候要用到的
    task_t task;
    task_info_t taskinfo;
    task.info = &taskinfo;

    char buf[4]; //用来放选项
    bool exit_flag = false;
    while (!exit_flag) {
        scanf("%s", buf);
        switch (atoi(buf)) {
        case 1:
            printf("任务发送\n");
            send_task(pool, &task);
            break;

        case 2:
            printf("员工注册\n");
            break;

        case 3:
            printf("员工休息\n");
            break;

        case 4:
            exit_flag = true;
            break;

        default:
            printf("输入错误,重新输入\n");
            break;
        }
    }

    free(pool);

    return 0;
}
