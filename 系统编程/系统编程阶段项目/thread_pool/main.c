#include "thread_pool.h"

//manlist：跑腿人员链表，user_info：当前用户下单信息
void mytask(struct running_man* man_list, struct user_info* userinfo)
{
    struct running_man* ptr;

    printf("下单的人名字叫做:%s\n", userinfo->name);

    for (ptr = man_list->next; ptr != NULL; ptr = ptr->next) {
        if (pthread_self() == ptr->tid) {
            printf("跑腿人员%s已接单\n", ptr->name);
            break;
        }
    }
}

int main(void)
{
    char buffer[4];
    struct user_info uinfo;
    struct running_man runman;

    // 1, initialize the pool
    thread_pool* pool = malloc(sizeof(thread_pool));
    init_pool(pool, 0);

    while (1) {
        scanf("%s", buffer);

        switch (atoi(buffer)) {
        case 1:
            printf("请输入用户名字及电话号码\n");
            //用户下单
            //1，从键盘当中获取用户数据
            scanf("%s", uinfo.name);
            scanf("%s", uinfo.phone_numb);
            //2，添加任务到线程池中
            add_task(pool, mytask, &uinfo);
            break;

        case 2:
            printf("请输入跑腿人员名字及电话号码\n");
            //注册跑腿人员
            //1，从键盘当中获取跑腿人员数据
            scanf("%s", runman.name);
            scanf("%s", runman.phone_numb);
            //2，添加跑腿人员信息到线程池中，并且创建对应的线程
            add_thread(pool, &runman);
            break;
        }
    }

    // 7, destroy the pool
    destroy_pool(pool);

    free(pool);

    return 0;
}
