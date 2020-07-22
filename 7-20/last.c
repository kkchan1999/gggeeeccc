#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct node {
    pthread_t id;
    char* name; //暂时这么搞吧
    struct node* next;
} listnode;

typedef struct head {
    int num;
    listnode* first_node;
    listnode* last_node;
} headnode;

void* routine(void* arg)
{
    // pthread_t tid = *(unsigned long*)arg;
    while (1) {
        sleep(3);
        printf("hello\n");
    }
}

int create(headnode* head)
{
    //判断有多少个线程
    if (head->num >= 10) {
        printf("线程太多了\n");
        return -1;
    }

    //创一个线程
    pthread_t tid;
    pthread_create(&tid, NULL, routine, NULL);

    listnode* node = calloc(1, sizeof(listnode));
    node->id = tid;
    node->name = "routine";
    node->next = NULL;

    if (head->first_node == NULL) {
        head->first_node = node;
    } else {
        listnode* temp = head->first_node;
        while (temp->next != NULL) {
            temp = temp->next; //跑到最后
        }
        temp->next = node;
    }

    head->num++;
}

int quit(headnode* head)
{
    //判断还有没有
    if (head->num == 0) {
        printf("num == 0\n");
        return -1;
    } else if (head->first_node == NULL) {
        printf("first == NULL\n");
        return -1;
    }

    //找到最前面的节点
    listnode* temp = head->first_node;
    head->first_node = head->first_node->next;
    pthread_cancel(temp->id);
    free(temp);
    head->num--;
}

int main(void)
{
    //整个带头节点的链表
    headnode* head = calloc(1, sizeof(headnode));
    head->last_node = NULL;
    head->first_node = NULL;
    head->num = 0;
    char* str = calloc(1, 64);
    while (1) {
        //检测键盘
        scanf("%s", str);
        if (strcmp(str, "create") == 0) {
            //如果检测到create
            create(head);
        } else if (strcmp(str, "exit") == 0) {
            //检测exit
            quit(head); //退出函数
            if (head->num == 0) {
                printf("没东西了，退出。\n");
                break;
            }
            printf("目前有%d个线程\n", head->num);

        } else {
            printf("%s\n", str);
        }
    }

    return 0;
}