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
#include <sys/time.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

typedef struct fd_list {
    int fd;
    struct fd_list* next;
    struct sockaddr_in client_addr;
} fd_list_t;

int main(int argc, char const* argv[])
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("申请socket失败");
        return -1;
    }

    struct sockaddr_in native_addr;
    native_addr.sin_family = AF_INET;
    native_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    native_addr.sin_port = htons(6666);

    if (bind(sockfd, (struct sockaddr*)&native_addr, sizeof(native_addr)) == -1) {
        perror("绑定套接字失败");
        goto bind_socket_err;
    }

    if (listen(sockfd, 50) == -1) {
        perror("设置最大连接失败");
        goto set_listen_err;
    }

    fd_set read_fd_set; //文件描述符集合

    char buf[1024];

    //弄个链表
    fd_list_t* list = malloc(sizeof(fd_list_t));
    list->fd = sockfd;
    list->next = NULL;
    list->client_addr = native_addr;

    while (1) {

        FD_ZERO(&read_fd_set);
        int max_fd = 0;
        bzero(buf, sizeof(buf));

        //for循环FD_SET
        for (fd_list_t* p = list; p != NULL; p = p->next) {
            FD_SET(p->fd, &read_fd_set);
            if (p->fd > max_fd) {
                max_fd = p->fd;
            }
            printf("添加了fd：%d\n", p->fd);
        }
        FD_SET(0, &read_fd_set); //键盘

        int retval = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL);
        if (retval == -1) {
            perror("监听异常");
            goto select_err;
        } else if (retval) {
            for (fd_list_t* p = list; p != NULL; p = p->next) {
                if (FD_ISSET(p->fd, &read_fd_set)) {
                    if (p->fd == sockfd) { //有新的客户端要连接
                        fd_list_t* new = malloc(sizeof(fd_list_t));
                        new->next = NULL;
                        socklen_t sklen = sizeof(new->client_addr);
                        new->fd = accept(sockfd, (struct sockaddr*)&new->client_addr, &sklen);
                        if (new->fd == -1) {
                            perror("客户端连接失败");
                            close(new->fd);
                            free(new);
                            return -1;
                        }

                        //插尾
                        fd_list_t* ptr;
                        for (ptr = list; ptr->next != NULL; ptr = ptr->next) {
                        }
                        ptr->next = new;
                        printf("新服务器连接成功！ip：%s,port:%u\n", inet_ntoa(new->client_addr.sin_addr), htons(new->client_addr.sin_port));
                    } else { //其他情况，即有信息发过来了
                        ssize_t recv_size;
                        recv_size = recv(p->fd, buf, sizeof(buf), 0);
                        if (recv_size == -1) {
                            perror("接收数据异常");
                            continue;
                        } else if (recv_size == 0) {
                            //断开了连接
                            printf("客户端断开连接\n");
                            fd_list_t* temp;
                            for (temp = list; temp->next != p; temp = temp->next) {
                            } //找到p的前一个
                            temp->next = p->next;
                            free(p);
                        } else {
                            printf("客户端信息：%s\n", buf);
                            char msg[1024];
                            sprintf(msg, "ip:%s的信息：%s", inet_ntoa(p->client_addr.sin_addr), buf);
                            for (fd_list_t* ptr = list; ptr != NULL; ptr = ptr->next) {
                                if (ptr->fd == sockfd || ptr->fd == p->fd) {
                                    continue;
                                }
                                recv_size = send(ptr->fd, msg, strlen(msg), 0);
                                if (recv_size == -1) {
                                    perror("发送错误！");
                                    continue;
                                }
                            }
                        }
                    }
                    //找了很久的....bug
                    break;
                }
            }
        }
    }

    return 0;

// socket_recv_err:
// close(client_fd);
client_connect_err:

select_err:
set_listen_err:
bind_socket_err:
    close(sockfd);
    return -1;
}
