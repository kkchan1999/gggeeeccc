#include "pool.h"

/* 
系统编程的阶段项目:基于线程池的人员分配系统
    基本上就是线程池的一些应用, 从文件读取staff后添加第二个任务会出现段错误,目前没找到bug在哪里,不打算修复了
 */

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
    ptr->onwork = true;
    //sleep(tasklist->info->time); //自动停止的情况

    //手动停止要给个信号量
    sem_wait(&ptr->sem);

    ptr->onwork = false;
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

    printf("该用户是vip吗?");
    scanf("%s", buf);
    int vip = atoi(buf);
    if (vip != 0 && vip != 1) {
        printf("输入错误！\n");
        return;
    } else {
        task->info->vip = vip;
    }

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

    staff_info_t* ptr = NULL;
    printf("有以下员工空闲：\n");
    for (ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        if (ptr->onwork == false) {
            printf("%s ", ptr->name);
        }
    }
    // if (ptr == NULL) {
    //     printf("全部员工都在干活,暂时不能休息!\n");
    //     return false;
    // }

    printf("\n请输入谁要休息：");
    char buf[64];
    scanf("%s", buf);

    ptr = NULL;
    staff_info_t* prev = NULL;
    for (ptr = pool->staff_info_list->next, prev = pool->staff_info_list; ptr != NULL; prev = ptr, ptr = ptr->next) {
        if (strcmp(buf, ptr->name) == 0 && ptr->onwork == false) {
            pthread_mutex_lock(&pool->lock);
            prev->next = ptr->next; //修改链表
            pthread_mutex_unlock(&pool->lock);
            pthread_cancel(ptr->tid); //取消线程
            pthread_join(ptr->tid, NULL); //接合线程

            sem_destroy(&ptr->sem); //销毁信号量

            printf("%ld号员工:%s已经休息了，他一共赚了%d元\n", ptr->tid, ptr->name, ptr->money);
            print_staff(ptr);
            pool->active_threads--;

            free(ptr); //释放资源
            return true;
        }
    }

    printf("输入有误，请重试\n");
    return false;
}

bool finsh_task(thread_pool_t* pool)
{
    if (pool->staff_info_list->next == NULL) {
        printf("没人上班！\n");
        return false;
    }

    printf("有以下员工在干活：\n");
    for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        if (ptr->onwork == true) {
            printf("%s ", ptr->name);
        }
    }

    printf("\n请输入要完成任务的员工名:");
    char buf[256];
    scanf("%s", buf);
    for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        if (strcmp(buf, ptr->name) == 0 && ptr->onwork == true) {
            printf("%s完成任务\n", ptr->name);
            sem_post(&ptr->sem);
            return true;
        }
    }
    printf("输入有误，请重试\n");
    return false;
}

bool add_staff_from_file(thread_pool_t* pool)
{
    printf("请输入文件路径:");
    char buf[256];
    scanf("%s", buf);

    FILE* f = fopen(buf, "r");
    if (f == NULL) {
        printf("打开失败!\n");
        return -1;
    }
    int count = 0;
    while (1) {
        staff_info_t* staff = malloc(sizeof(staff_info_t));
        if (read_staff(f, staff) == false) {
            break;
        }
        staff->onwork = false;
        staff->next = NULL;
        add_staff(pool, staff);
        count++;
    }
    printf("添加了%d个员工\n!", count);
    fclose(f);
    return 0;
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
        return false;
    } else {
        ptr->sex = sex;
    }

    ptr->money = 0; //新员工还想有钱？
    ptr->next = NULL;

    add_staff(pool, ptr);

    return true;
}

void show_staff(thread_pool_t* pool)
{
    for (staff_info_t* ptr = pool->staff_info_list->next; ptr != NULL; ptr = ptr->next) {
        char sex[16];
        ptr->sex ? strcpy(sex, "男") : strcpy(sex, "女");
        printf("姓名:%s,电话号码:%s,性别:%s,工资:%d\n", ptr->name, ptr->phone, sex, ptr->money);
    }
}

int main(int argc, char const* argv[])
{
    //初始化
    thread_pool_t* pool = malloc(sizeof(thread_pool_t));
    init_pool(pool, 0);

    char buf[4]; //用来放选项
    bool exit_flag = false;
    while (!exit_flag) {
        printf("\n*******************\n");
        printf("请选择选项:\n1.发送任务\n2.员工注册\n3.手动完成任务\n4.员工休息\n5.查看员工\n6.查看剩余任务数\n7.从文件录入员工(内侧版)\n0.退出系统\n\n");
        scanf("%s", buf);
        switch (atoi(buf)) {
        case 1:
            send_task(pool);
            break;

        case 2:
            input_staff(pool);
            break;

        case 3:
            finsh_task(pool);
            break;

        case 4:
            staff_rest(pool);
            break;

        case 0:
            if (destroy_pool(pool)) {
                exit_flag = true;
            }
            break;

        case 5:
            show_staff(pool);
            break;

        case 6:
            printf("现在有%d个任务没做完\n", pool->watting_tasks);
            break;

        case 7:
            add_staff_from_file(pool);
            break;

        default:
            printf("输入错误,重新输入\n");
            break;
        }
    }

    free(pool);

    return 0;
}
