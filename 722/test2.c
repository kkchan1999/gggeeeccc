/* 程序创建3条线程，并且每条线程都去改变全局变量count，当count为0的时候让所有的线程睡眠，等待我们的中断信号（ctrl+c），
	如果接受这个信号则count+1，并且唤醒一条线程去继续改变count，当输入exit时结束所有的子线程，并且退出整个程序
 */

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int count = 10;

pthread_cond_t cond;
pthread_mutex_t m;

void sighand(int signum)
{
    count += 1;
    //唤醒所有线程
    pthread_cond_signal(&cond);
}

void* routine(void* arg)
{
    pthread_mutex_unlock(&m);
}
void* thread(void* arg)
{
    pthread_cleanup_push(routine, NULL);
    while (1) {
        //加锁
        pthread_mutex_lock(&m);
        if (count == 0) {
            //没钱了，睡觉
            pthread_cond_wait(&cond, &m); //被唤醒后自动加上锁(好像)
        }
        count -= 1;
        printf("%ld用了1元，还剩%d\n", *(pthread_t*)arg, count);
        //解锁
        pthread_mutex_unlock(&m);

        sleep(1);
    }
    pthread_cleanup_pop(0);
}

int main(int argc, char const* argv[])
{
    //初始化
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&m, NULL);

    //信号设置
    signal(SIGINT, sighand);

    //线程属性设置
    // pthread_attr_t t;
    // pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
    // pthread_attr_init(&t);

    //开启线程
    pthread_t tid[3];
    pthread_create(tid, NULL, thread, (void*)tid);
    pthread_create(tid + 1, NULL, thread, (void*)(tid + 1));
    pthread_create(&tid[2], NULL, thread, (void*)(tid + 2));

    //检测输入
    char buf[64];
    while (strcmp(buf, "exit") != 0) {
        scanf("%s", buf);
    }

    //取消线程
    pthread_cancel(tid[0]);
    pthread_cancel(tid[1]);
    pthread_cancel(tid[2]);

    //回收线程
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);

    //销毁操作
    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cond);
    // pthread_attr_destroy(&t);

    printf("收工\n");
    return 0;
}
