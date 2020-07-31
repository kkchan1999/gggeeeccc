#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct list {
    char* ip;
    struct list* next;
} list_t;

int main(int argc, char const* argv[])
{
    //要用的东西申请好
    int sockfd;
    struct sockaddr_in dest_addr, recv_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    ssize_t send_size, recv_size;
    char buf[1024];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("申请套接字失败");
        return -1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(argv[1]);
    dest_addr.sin_port = htons(atoi(argv[2]));

    //bind
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(atoi(argv[3]));
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) == -1) {
        perror("设置套接字选项失败");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == -1) {
        perror("绑定ip地址和端口失败");
        close(sockfd);
        return -1;
    }
    send_size = sendto(sockfd, "QQ小冰上线了\n", 19, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (send_size == -1) {
        perror("发送失败！");
        return -1;
    }

    fd_set rset;
    int retval;

    list_t* list = malloc(sizeof(list_t));
    list->ip = "192.168.33.54";
    list->next = NULL;

    // int friends_num = 1;

    while (1) {
        FD_ZERO(&rset);
        FD_SET(0, &rset);
        FD_SET(sockfd, &rset);

        retval = select(sockfd + 1, &rset, NULL, NULL, NULL);

        if (retval == -1) {
            perror("select failed!");
            return -1;
        } else if (retval) {

            if (FD_ISSET(sockfd, &rset)) {

                send_size = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&recv_addr, &socklen);
                if (send_size == -1) {
                    perror("接收udp数据失败\n");
                    break;
                }

                if (strcmp(inet_ntoa(recv_addr.sin_addr), "192.168.33.54") == 0) {
                    continue; //自己发的信息，直接跳过
                }

                int num = 0;
                list_t* ptr;
                for (ptr = list; ptr != NULL; ptr = ptr->next) {

                    if (strcmp(inet_ntoa(recv_addr.sin_addr), ptr->ip) == 0) {
                        num++;
                    }
                }
                if (num == 0) {
                    //新好友？
                    printf("发送了欢迎信息，ptr:%s dest:%s\n", ptr->ip, inet_ntoa(recv_addr.sin_addr));
                    send_size = sendto(sockfd, "新年好，没烦恼", 24, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    if (send_size == -1) {
                        perror("发送失败！");
                        break;
                    }
                    list_t* new = malloc(sizeof(list_t));
                    new->ip = inet_ntoa(recv_addr.sin_addr);
                    new->next = NULL;

                    ptr = list;
                    while (ptr->next != NULL) {
                        ptr = ptr->next;
                    } //跑到最后
                    ptr->next = new;
                }

                printf("收到ip：%s的信息: %s\n", inet_ntoa(recv_addr.sin_addr), buf);

            } else if (FD_ISSET(0, &rset)) {
                //发送数据
                printf("发送数据\n");

                scanf("%s", buf);

                send_size = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));

                if (send_size == -1) {
                    perror("发送失败！");
                    break;
                }
            }
        }
    }

    close(sockfd);

    return 0;
}
