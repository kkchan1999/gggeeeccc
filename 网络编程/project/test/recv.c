#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct client_info
{
    int client_fd;
    char name[256];
    char ip[16];
    short port;
    pthread_t tid;

    struct client_info *next;
} client_info_node;

int main(int argc, char const *argv[])
{
    int sockfd;
    int retval;
    //绑定一些信息
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //ipv4 tcp
    if (sockfd == -1)
    {
        perror("申请套接字失败");
        return -1;
    }

    struct sockaddr_in native_addr;

    native_addr.sin_family = AF_INET;                //指定引用IPV4的协议
    native_addr.sin_port = htons(6666);              //指定端口号，转化为网络字节序（大端序）
    native_addr.sin_addr.s_addr = htonl(INADDR_ANY); //将所有的IP地址转化为二进制的网络字节序的数据进行绑定

    retval = bind(sockfd, (struct sockaddr *)&native_addr, sizeof(native_addr));
    if (retval == -1)
    {
        perror("绑定套接字地址失败");
        goto bind_socket_err;
    }

    retval = listen(sockfd, 1); //最多一个
    if (retval == -1)
    {
        perror("设置最大连接数失败");
        goto set_listen_err;
    }

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t sklen = sizeof(client_addr);

    printf("开始等待连接\n");

    client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sklen); //阻塞等待连接
    if (client_fd == -1)
    {
        perror("客户端链接失败");
        goto client_connect_err;
    }
    printf("服务器：客户端连接成功\n");

    char buf[256];

    //先创建一个文件

    FILE *f = fopen("./321.png", "w+");
    int count = 0;
    while (1)
    {
        bzero(buf, sizeof(buf));
        retval = recv(client_fd, buf, 256, 0);
        if (retval == -1)
        {
            perror("读取错误!");
            goto socket_recv_err;
        }
        else if (retval == 0)
        {
            printf("连接断开了.\n");
            break;
        }
        else
        {
            fwrite(buf, 1, retval, f);
            count += retval;
        }
    }
    printf("写完了?%d\n", count);
    fclose(f);
    close(client_fd); //关闭客户端通信
    close(sockfd);    //关闭服务器的socket资源

    return 0;

socket_recv_err:
    close(sockfd);
    fclose(f);
client_connect_err:
set_listen_err:
bind_socket_err:
    close(sockfd);
    return -1;
}
