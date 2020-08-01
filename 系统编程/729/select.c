#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>

int main(void)
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

    struct sockaddr_in native_addr;

    native_addr.sin_family = AF_INET; //指定引用IPV4的协议

    native_addr.sin_port = htons(6666); //指定端口号，转化为网络字节序（大端序）

    //inet_aton(INADDR_ANY, &(native_addr.sin_addr));

    native_addr.sin_addr.s_addr = htonl(INADDR_ANY); //将所有的IP地址转化为二进制的网络字节序的数据进行绑定

    /*
		将指定的地址信息及本程序的套接字绑定在一起
		skt_fd：套接字的文件描述符
		&native_addr：需要绑定的地址信息结构体（每个协议的地址信息结构体是不一样的，我们用IPV4的协议便需要引用struct sockaddr_in）
		sizeof(native_addr)：传入的结构体长度
	*/
    retval = bind(skt_fd, (struct sockaddr*)&native_addr, sizeof(native_addr));
    if (retval == -1) {
        perror("绑定套接字地址失败");
        goto bind_socket_err;
    }

    /*
		设置套接字的同时通信最大连接数为50，并且将这个套接字的属性设置为可监听属性
	*/
    retval = listen(skt_fd, 50);
    if (retval == -1) {
        perror("设置最大连接数失败");
        goto set_listen_err;
    }

    int max_fd = skt_fd; //拿到要操作的文件描述符中最大文件描述符
    fd_set read_fd_set; //读文件描述符集合

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t sklen = sizeof(client_addr);

    printf("start listen network\n");

    char buffer[1024];

    while (1) {
        FD_ZERO(&read_fd_set); //清空一下文件描述符集合
        FD_SET(skt_fd, &read_fd_set);
        FD_SET(0, &read_fd_set);

        /*多路复用函数：实现多个文件描述符监听的功能，如果有文件描述符符合条件唤醒这个函数，则其他文件描述符会自动清除出来，只保留唤醒select函数的文件描述符在集合里面
			max_fd+1:要监控的最大的文件描述符值+1
			&read_fd_set：读文件描述符集合
			NULL, NULL, NULL：分别代表写文件描述符集合，其他类型文件描述符集合，时间参数结构体		
		*/
        retval = select(max_fd + 1, &read_fd_set, NULL, NULL, NULL); //这个函数将会在这里阻塞，一直等到我们我监听的文件描述符有动静才会被唤醒
        if (retval == -1) {
            perror("监听异常");
            goto select_err;
        } else if (retval) {
            //判断文件描述符是不是在这个集合里面，如果还存在在这个集合里面则证明是skt_fd这个文件描述符唤醒的我们
            if (FD_ISSET(skt_fd, &read_fd_set)) {
                client_fd = accept(skt_fd, (struct sockaddr*)&client_addr, &sklen);
                if (client_fd == -1) {
                    perror("客户端链接失败");
                    goto client_connect_err;
                }

                printf("服务器：客户端连接成功\n");
                printf("客户端信息：\n客户端IP为%s，端口号为%hu\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            } else if (FD_ISSET(0, &read_fd_set)) {
                read(0, buffer, sizeof(buffer));
                printf("从键盘中读取到的数据为%s\n", buffer);
            }

        } else
            break;
    }

#if 0	

	/*
		等待客户端链接，链接成功后返回一个代表客户端通信的文件描述符,具备阻塞特性
		skt_fd：代表套接字接口
		client_addr：链接成功后客户端的地址信息会存放到这里面
		sklen：代表结构体的长度
	*/
	

	char buffer[1024];
	ssize_t recv_size;

	while(1)
	{
		bzero(buffer, sizeof(buffer));

		/*
			接受来自与客户端的数据，这个接受具备阻塞特性
			跟read函数差不多，都是指定从client_fd里面读取sizeof(buffer)长的数据放到buffer当中，0则代表按照默认操作读取数据
		*/
		recv_size = recv(client_fd, buffer, sizeof(buffer), 0);
		if(recv_size == -1)
		{
			perror("接受数据异常");
			goto socket_recv_err;
		}
		else if(recv_size == 0)//代表客户端断开连接
			break;

		printf("接收到来自与客户端%ld个字节的数据：%s\n", recv_size, buffer);
	}

	close(client_fd);//关闭客户端通信
	close(skt_fd);//关闭服务器的socket资源

#endif

    return 0;

socket_recv_err:
    close(client_fd);
client_connect_err:

select_err:
set_listen_err:
bind_socket_err:
    close(skt_fd);
    return -1;
}
