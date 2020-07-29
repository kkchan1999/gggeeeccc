#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>


//./client 192.168.33.3
int main(int argc, const char *argv[])
{
	int skt_fd;
	int retval;

	/*
		获取程序通信的套接字（接口）
		AF_INET：IPV4的协议
		SOCK_STREAM：指定TCP协议
		0：代表不变化协议内部（ip手册中指定的参数）
	*/
	skt_fd = socket( AF_INET, SOCK_STREAM, 0);
	if(skt_fd == -1)
	{
		perror("申请套接字失败");
		return -1;
	}

	struct sockaddr_in srv_addr;

	srv_addr.sin_family = AF_INET;//指定引用IPV4的协议

	srv_addr.sin_port = htons(6666);//指定端口号，转化为网络字节序（大端序）

	srv_addr.sin_addr.s_addr = inet_addr(argv[1]);//将所有的IP地址转化为二进制的网络字节序的数据进行绑定	

	retval = connect(skt_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
	if(retval == -1)
	{
		perror("客户端连接到服务器失败\n");
		goto connect_server_err;
	}

	printf("客户端：连接服务器成功\n");

	char buffer[1024];
	ssize_t send_size;

	while(1)
	{
		scanf("%s", buffer);

		send_size = send( skt_fd, buffer, strlen(buffer), 0);
		if(send_size == -1)
			break;
	}


	close(skt_fd);

	return 0;


connect_server_err:
	close(skt_fd);
	return -1;
}
