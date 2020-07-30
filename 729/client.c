#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

void* recv_data(void* arg)
{
    int skt_fd = *(int*)arg;
    char buffer[1024];
    ssize_t recv_size;

    while (1) {
        bzero(buffer, sizeof(buffer));

        /*
			接受来自与客户端的数据，这个接受具备阻塞特性
			跟read函数差不多，都是指定从client_fd里面读取sizeof(buffer)长的数据放到buffer当中，0则代表按照默认操作读取数据
		*/
        recv_size = recv(skt_fd, buffer, sizeof(buffer), 0);
        if (recv_size == -1) {
            perror("接受数据异常");
            break;
        } else if (recv_size == 0) //代表客户端断开连接
        {
            //删除链表节点
            break;
        }

        printf("接收到来自与客户端%ld个字节的数据：%s\n", recv_size, buffer);
    }

    return NULL;
}

//./client 192.168.33.3
int main(int argc, const char* argv[])
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
    if (skt_fd == -1) {
        perror("申请套接字失败");
        return -1;
    }

    struct sockaddr_in srv_addr;

    srv_addr.sin_family = AF_INET; //指定引用IPV4的协议

    srv_addr.sin_port = htons(6666); //指定端口号，转化为网络字节序（大端序）

    srv_addr.sin_addr.s_addr = inet_addr(argv[1]); //将所有的IP地址转化为二进制的网络字节序的数据进行绑定

    retval = connect(skt_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    if (retval == -1) {
        perror("客户端连接到服务器失败\n");
        goto connect_server_err;
    }

    printf("客户端：连接服务器成功\n");

    char buffer[1024];
    ssize_t send_size;
    pthread_t tid;

    pthread_create(&tid, NULL, recv_data, &skt_fd);

    while (1) {
        scanf("%s", buffer);

        send_size = send(skt_fd, buffer, strlen(buffer), 0);
        if (send_size == -1)
            break;
    }

    close(skt_fd);

    return 0;

connect_server_err:
    close(skt_fd);
    return -1;
}
