#include "pool.h"

void mytask(void* arg)
{
    //这个是啥
}

int main(int argc, char const* argv[])
{
    //初始化
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    init_pool(pool, 3);

    //菜单
    char buf[4];
    bool exit_flag = false;
    while (!exit_flag) {
        scanf("%s", buf);
        switch (atoi(buf)) {
        case 1:
            printf("任务发送\n");
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
