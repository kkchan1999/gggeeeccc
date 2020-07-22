#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct thread_node {
    pthread_t tid;
    void*(thread_name)(void*);
    void* arg;

    struct thread_node* next;
} thread_node_t;

thread_node_t* request_init_list_node(pthread_t tid, void*(thread_name)(void*), void* arg)
{
}