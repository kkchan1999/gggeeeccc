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

int main(int argc, char const *argv[])
{
    int skt_fd;
    int retval;

    /*
		获取程序通信的套接字（接口）
		AF_INET：IPV4的协议
		SOCK_STREAM：指定TCP协议
		0：代表不变化协议内部（ip手册中指定的参数）
	*/
    skt_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (skt_fd == -1)
    {
        perror("申请套接字失败");
        return -1;
    }

    struct sockaddr_in srv_addr;

    srv_addr.sin_family = AF_INET; //指定引用IPV4的协议

    srv_addr.sin_port = htons(6666); //指定端口号，转化为网络字节序（大端序）

    srv_addr.sin_addr.s_addr = inet_addr(argv[1]); //将所有的IP地址转化为二进制的网络字节序的数据进行绑定

    retval = connect(skt_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    if (retval == -1)
    {
        perror("客户端连接到服务器失败\n");
        goto connect_server_err;
    }

    FILE *f = fopen("./123.png", "r");
    char buf[256];
    int count = 0;
    while (1)
    {
        bzero(buf, sizeof(buf));
        retval = fread(buf, 1, 256, f);
        if (retval == 0)
        {
            break;
        }
        send(skt_fd, buf, retval, 0);
        count += retval;
    }
    printf("读了%d\n", count);
    fclose(f);
    close(skt_fd);

    return 0;
connect_server_err:
    close(skt_fd);
    return -1;
}
