#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

sem_t sem;

char buffer[1024];

void* thread(void* arg)
{

    while (1) {
        sem_wait(&sem); //p操作
        printf("buffer=%s\n", buffer);
    }

    return NULL;
}

int main(void)
{

    pthread_t tid;

    sem_init(&sem, 0, 0); //信号量初始化，第一个0代表在线程间操作，第二个0代表初始化初值为0

    pthread_create(&tid, NULL, thread, NULL);

    while (1) {
        scanf("%s", buffer);
        sem_post(&sem); //v操作
    }

    pthread_join(tid, NULL);

    sem_destroy(&sem);

    return 0;
}
