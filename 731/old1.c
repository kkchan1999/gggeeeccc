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

int main(int argc, char const* argv[])
{
    int sockfd;
    struct sockaddr_in native_addr, dest_addr, recv_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("申请套接字失败");
        goto request_socket_err;
    }

    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag)) == -1) {
        perror("设置套接字选项失败");
        goto setsockopt_err;
    }

    native_addr.sin_family = AF_INET;
    native_addr.sin_port = htons(atoi(argv[1])); //自己监听的端口
    native_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr*)&native_addr, sizeof(native_addr)) == -1) {
        perror("绑定对应的ip地址及端口失败");
        goto bind_socket_err;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = atoi(argv[2]); //发送的端口
    dest_addr.sin_addr.s_addr = inet_addr("192.168.33.255");
    printf("发送上线信息\n");
    ssize_t send_size;
    send_size = sendto(sockfd, "爷上线了\n", 14, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (send_size == -1) {
        printf("发送失败！\n");
    }
    printf("发完了\n");
    char buf[256];
    ssize_t recv_size;
    int skt_len = sizeof(recv_addr);
    while (1) {
        bzero(buf, sizeof(buf));
        sendto(sockfd, "爷上线了\n", 14, 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        // recv_size = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&recv_addr, &skt_len);
        // if (recv_size == -1) {
        //     perror("接受UDP数据失败\n");
        //     continue;
        // } else {
        //     sendto(sockfd, "新年好，没烦恼", 22, 0, (struct sockaddr*)&recv_addr, sizeof(recv_addr));
        // }
        sleep(1);
    }

    return 0;

setsockopt_err:
bind_socket_err:
    close(sockfd);
request_socket_err:
    return -1;
}
