#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

//遍历动作
#define list_for_each(list_head, ptr) \
    for (ptr = list_head->next; ptr != NULL; ptr = ptr->next)

typedef struct thread_node {

    pthread_t tid;
    void* (*thread_name)(void*);
    void* arg;

    struct thread_node* next;

} thread_node_t;

thread_node_t* request_and_init_thread_list_node(pthread_t tid, void*(thread_name)(void*), void* arg)
{
    thread_node_t* new_node;

    new_node = malloc(sizeof(thread_node_t));
    if (new_node == NULL) {
        perror("申请节点失败");
        return NULL;
    }

    new_node->tid = tid;
    new_node->thread_name = thread_name;
    new_node->arg = arg;

    new_node->next = NULL;

    return new_node;
}

void thread_list_insert_node(thread_node_t* list_head, thread_node_t* insert_node)
{
    insert_node->next = list_head->next;
    list_head->next = insert_node;
}

bool remove_and_destroy_thread_list_node(thread_node_t* list_head)
{
    thread_node_t *prev_ptr, *ptr;

    for (prev_ptr = list_head, ptr = list_head->next; ptr != NULL; prev_ptr = ptr, ptr = ptr->next) {
        if (ptr->next == NULL) {
            prev_ptr->next = NULL; //将节点移除出链表
            pthread_cancel(ptr->tid); //消灭线程
            free(ptr); //释放节点
            return true;
        }
    }

    printf("没有找到可以杀死的线程\n");

    return false;
}

void destroy_thread_list(thread_node_t* list_head)
{
    while (remove_and_destroy_thread_list_node(list_head))
        ;

    free(list_head);
}

void* function(void* arg)
{
    printf("create thread arg=%s", (char*)arg);

    while (1)
        sleep(100);

    return NULL;
}

int main(void)
{

    char input_string[256];
    thread_node_t *list_head, *new_node, *ptr;
    pthread_t tid;
    long count = 0;

    list_head = request_and_init_thread_list_node(0, NULL, NULL);

    while (1) {
        scanf("%s", input_string);

        if (strcmp("create", input_string) == 0) {
            pthread_create(&tid, NULL, function, (void*)count);

            new_node = request_and_init_thread_list_node(tid, function, (void*)count);

            thread_list_insert_node(list_head, new_node);

            count++;
        } else if (strcmp("exit", input_string) == 0) {
            remove_and_destroy_thread_list_node(list_head);
        } else {
            list_for_each(list_head, ptr)
            {
                printf("destroy %ld\n", (long)ptr->arg);
            }

            break;
        }
    }

    destroy_thread_list(list_head);

    return 0;
}
