#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

typedef struct info {
    pthread_t tid;
    struct sockaddr_in client_addr;
    int client_fd;
    unsigned int id;
    char name[256];
    struct info* next;
    struct info* head;

} client_info_t;

void send_all(client_info_t* head, char* msg, client_info_t* src)
{
    for (client_info_t* p = head; p != NULL; p = p->next) {
        if (p != src) {
            send(p->client_fd, msg, strlen(msg), 0);
        }
    }
}

void* my_recv(void* arg)
{
    client_info_t* p = (client_info_t*)arg;
    int client_fd = p->client_fd;

    p->tid = pthread_self();

    bool id = false;
    bool name = false;

    char buffer[512];
    char msg[1024];
    ssize_t recv_size;
    while (1) {
        bzero(buffer, sizeof(buffer));
        bzero(msg, sizeof(msg));

        // if (id != true) {
        //     send(p->client_fd, "请输入账号", 16, 0);
        // }
        if (name != true) {
            send(p->client_fd, "请输入name", 16, 0);
        }

        recv_size = recv(client_fd, buffer, sizeof(buffer), 0);
        if (recv_size == -1) {
            perror("接受数据异常");
            close(client_fd);
            pthread_exit(NULL);
        } else if (recv_size == 0) {
            //代表客户端断开连接
            break;
        }

        // if (id != true) {
        //     p->id = atoi(buffer);
        //     id = true;
        // }
        if (name != true) {
            strcpy(p->name, buffer);
            name = true;
        }

        if (name == true) {
            sprintf(msg, "[%u]%s:\n%s", ntohs(p->client_addr.sin_port), p->name, buffer);
            send_all(p->head, msg, p);
        }

        // printf("接收到来自与客户端%ld个字节的数据：%s\n", recv_size, buffer);
    }
    pthread_exit(NULL);
}

void* my_send(void* arg)
{
    client_info_t* ptr = (client_info_t*)arg;

    char buf[256];
    char msg[512];

    while (1) {
        bzero(buf, sizeof(buf));
        bzero(msg, sizeof(msg));
        scanf("%s", buf);
        if (strcmp(buf, "exit") == 0) {
            pthread_exit(NULL); //怎么在这里退出呢
        }

        sprintf(msg, "管理员信息: %s\n", buf);
        //发送给所有人
        // for (client_info_t* p = ptr; p != NULL; p = p->next) {
        //     send(p->client_fd, msg, strlen(msg), 0);
        // }
        send_all(ptr, msg, NULL);
    }
}

int main(int argc, char const* argv[])
{
    int sockfd;

    client_info_t list_head;
    list_head.next = NULL;

    //获取socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("申请socket失败");
        return -1;
    }

    //指定地址信息
    struct sockaddr_in native_addr;
    native_addr.sin_family = AF_INET; //ipv4
    //htons:转换16位整数为网络端序 htonl:32位
    native_addr.sin_port = htons(6666);
    native_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //绑定地址信息与申请socket
    if (bind(sockfd, (struct sockaddr*)&native_addr, sizeof(native_addr)) == -1) {
        perror("bind socket failed");
        goto bind_socket_err;
    }

    //将socket设置为监听模式，最大连接数为50
    if (listen(sockfd, 50) == -1) {
        perror("设置最大连接数失败");
        goto set_linten_err;
    }

    //开启发送线程
    pthread_create(&list_head.tid, NULL, my_send, &list_head);

    printf("开始等待连接!\n");

    //从这下面开始改动

    //accept会阻塞等待，可等到连接成功后开启线程

    while (1) {
        //等待客户端连接
        client_info_t* client = malloc(sizeof(client_info_t));
        client->head = &list_head;

        socklen_t sklen = sizeof(client->client_addr);
        client->client_fd = accept(sockfd, (struct sockaddr*)&client->client_addr, &sklen);

        if (client->client_fd == -1) {
            perror("客户端连接失败");
            close(client->client_fd);
            free(client);
            break;
        }

        printf("客户端连接成功\n客户端IP: %s, 端口号: %hu\n", inet_ntoa(client->client_addr.sin_addr), ntohs(client->client_addr.sin_port));

        //开启线程
        if (pthread_create(&client->tid, NULL, my_recv, (void*)client) == -1) {
            perror("创建线程失败!");
            close(client->client_fd);
            free(client);
            break;
        }

        //插尾
        client_info_t* p = &list_head;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = client;
    }

// client_connect_err:
//     close(client_fd);
set_linten_err:
bind_socket_err:
    close(sockfd);
    return 0;
}
