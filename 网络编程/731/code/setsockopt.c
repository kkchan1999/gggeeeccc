#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

int client_fd;

void sighand(int signum)
{
	char buffer[1024]={0};
	ssize_t recv_size;

	

	
	//带外数据包每次接受仅有一个字节
	recv_size = recv(client_fd, buffer, sizeof(buffer), MSG_OOB);
	if(recv_size == -1)
	{
		perror("接受数据异常");
		return;
	}
	
	printf("这是一个带外数据包,%ld个字节的数据：%s\n", recv_size, buffer);
	
	
	
}


int main(void)
{
	int skt_fd;
	int retval;

	
	skt_fd = socket( AF_INET, SOCK_STREAM, 0);
	if(skt_fd == -1)
	{
		perror("申请套接字失败");
		return -1;
	}

	int sw = 1;

#if 1
	//设置地址及端口可以被复用的属性
	retval = setsockopt(skt_fd, SOL_SOCKET, SO_REUSEADDR, &sw, sizeof(sw));
	if(retval == -1)
	{
		perror("设置地址及端口复用失败");
		goto setsock_reuse_addr_err;
	}

#endif

	struct sockaddr_in native_addr;

	native_addr.sin_family = AF_INET;//指定引用IPV4的协议

	native_addr.sin_port = htons(6666);//指定端口号，转化为网络字节序（大端序）

	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);//将所有的IP地址转化为二进制的网络字节序的数据进行绑定

	
	retval = bind( skt_fd, (struct sockaddr *)&native_addr, sizeof(native_addr));
	if(retval == -1)
	{
		perror("绑定套接字地址失败");
		goto bind_socket_err;
	}

	int recv_low_size = 30;

	
	//设置接受数据大小的下限（也就是我们recv数据时要到多少个数据才会相应将其数据读取出来）（这个设置对select等多路复用是不生效的）
	retval = setsockopt(skt_fd, SOL_SOCKET, SO_RCVLOWAT, &recv_low_size, sizeof(recv_low_size));
	if(retval == -1)
	{
		perror("设置接收下限失败");
		goto setsock_recvlowat_err;
	}
	


	retval = listen(skt_fd, 50);
	if(retval == -1)
	{
		perror("设置最大连接数失败");
		goto set_listen_err;
	}

	
	struct sockaddr_in client_addr;
	socklen_t sklen = sizeof(client_addr);

	client_fd = accept(skt_fd, (struct sockaddr *)&client_addr, &sklen);
	if(client_fd == -1)
	{
		perror("客户端链接失败");
		goto client_connect_err;
	}

	printf("服务器：客户端连接成功\n");
	printf("客户端信息：\n客户端IP为%s，端口号为%hu\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	char buffer[1024];
	ssize_t recv_size;

	signal(SIGURG, sighand);//带外数据必须要设置SIGURG的相应（因为带外数据他默认会触发这个信号）

	fcntl(client_fd, F_SETOWN, getpid());//必须要设置文件描述符归属于哪一个进程才可以让属于文件描述符的信号出发到此进程中


	while(1)
	{
		bzero(buffer, sizeof(buffer));

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

	return 0;

socket_recv_err:
	close(client_fd);
client_connect_err:
set_listen_err:
bind_socket_err:
setsock_recvlowat_err:
setsock_reuse_addr_err:
	close(skt_fd);
	return -1;
}

