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

void show_friends(list_t* ptr)
{
    printf("有以下好友：");
    for (; ptr != NULL; ptr = ptr->next) {
        printf("%s\t", ptr->ip);
    }
    printf("\n搞定收工\n");
}

int main(int argc, char const* argv[])
{
    if (argc != 5) {
        printf("格式：自己的ip  自己的端口  别人的ip  别人的端口\n");
        return -1;
    }

    //要用的东西申请好
    int sockfd;
    struct sockaddr_in dest_addr, recv_addr, chat_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    ssize_t send_size, recv_size;
    char buf[1024];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("申请套接字失败");
        return -1;
    }

    list_t* list = malloc(sizeof(list_t)); //申请链表头，用来放自己的ip
    list->ip = (char*)argv[1]; //第1个参数是自己的ip
    list->next = NULL;

    printf("自己的ip：%s\n", list->ip);

    //bind
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(atoi(argv[2])); //第2参数是自己的端口
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(argv[3]); //这个是别人的ip
    dest_addr.sin_port = htons(atoi(argv[4])); //这个是别人的端口

    chat_addr.sin_family = AF_INET;
    chat_addr.sin_port = htons(atoi(argv[4])); //用同一个端口

    //开启广播模式
    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) == -1) {
        perror("设置套接字选项失败");
        return -1;
    }

    //绑定自己的地址和端口
    if (bind(sockfd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) == -1) {
        perror("绑定ip地址和端口失败");
        close(sockfd);
        return -1;
    }

    //上线的时候发一段通知
    send_size = sendto(sockfd, "QQ小冰上线了\n", 19, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (send_size == -1) {
        perror("发送失败！");
        return -1;
    } else if (send_size) {
        printf("发送了%ld字节的上线信息，19字节？\n", send_size);
    }

    //开始select多路重载
    fd_set rset;
    int retval;

    // int friends_num = 1;//这个好像暂时没用

    while (1) {
        FD_ZERO(&rset);
        FD_SET(0, &rset);
        FD_SET(sockfd, &rset);

        //因为没有申请其他的，所以sockfd是最大的描述符了
        retval = select(sockfd + 1, &rset, NULL, NULL, NULL);

        if (retval == -1) {
            perror("select failed!");
            return -1;

        } else if (retval) { //有描述符有事

            if (FD_ISSET(sockfd, &rset)) { //收到信息了

                char* temp = inet_ntoa(recv_addr.sin_addr);
                send_size = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&recv_addr, &socklen);
                if (send_size == -1) {
                    perror("接收udp数据失败\n");
                    break;
                }

                if (strcmp(inet_ntoa(recv_addr.sin_addr), list->ip) == 0) {
                    // printf("收到自己发的信息\n");
                    continue; //自己发的信息，直接跳过
                }

                //看看收到的信息之前有没有收过
                int num = 0;
                list_t* ptr;
                for (ptr = list; ptr != NULL; ptr = ptr->next) {
                    //遍历链表，每个ip去对比一下
                    if (strcmp(inet_ntoa(recv_addr.sin_addr), ptr->ip) == 0) {
                        num++; //有一样的才会加
                    }
                }

                /* 
                    这里面看起来是出了问题
                    新用户过来的时候会段错误，大概率是链表的问题(好像已经解决了)                                  
                 */
                if (num == 0) {
                    //新好友？
                    printf("发送了欢迎信息，dest:%s\n", inet_ntoa(recv_addr.sin_addr));
                    send_size = sendto(sockfd, "新年好，没烦恼", 24, 0, (struct sockaddr*)&recv_addr, sizeof(recv_addr));
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

                //打印别人发过来的信息，看起来
                printf("收到ip：%s的信息: %s\n", inet_ntoa(recv_addr.sin_addr), buf);

            } else if (FD_ISSET(0, &rset)) {
                //检测到键盘输入数据的情况
                int flag = 0; //用来判断是否退出
                list_t* ptr;
                scanf("%s", buf);
                switch (atoi(buf)) {
                case 1:
                    //查看list里面的ip
                    show_friends(list);
                    bzero(buf, sizeof(buf));
                    printf("请输入对方的ip：");
                    scanf("%s", buf);
                    chat_addr.sin_addr.s_addr = inet_addr(buf);

                    //这里加个判断
                    printf("请输入发送内容：");
                    bzero(buf, sizeof(buf));
                    scanf("%s", buf);
                    send_size = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&chat_addr, sizeof(chat_addr));
                    if (send_size == -1) {
                        perror("发送失败！");
                        flag = 1;
                    }

                    break;
                case 8888:
                    flag = 1;
                    printf("退出程序\n");
                    break;
                default:
                    send_size = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
                    if (send_size == -1) {
                        perror("发送失败！");
                        flag = 1;
                    }
                    break;
                }
                if (flag) {
                    break;
                }
            }
        }
        usleep(1000); //用sleep阻塞一下不知道有没有用
    }

    close(sockfd);

    return 0;
}
