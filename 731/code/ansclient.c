#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


struct msg_info{
	int pake_size;//4字节
	char name[16];//16字节
	char buffer[1024];
};

void *recv_data(void *arg)
{
	int skt_fd = *(int *)arg;
	struct msg_info recv_msg;
	ssize_t recv_size;

	while(1)
	{
		bzero(&recv_msg, sizeof(recv_msg));

		/*
			接受来自与客户端的数据，这个接受具备阻塞特性
			跟read函数差不多，都是指定从client_fd里面读取sizeof(buffer)长的数据放到buffer当中，0则代表按照默认操作读取数据
		*/
		recv_size = recv(skt_fd, &recv_msg, sizeof(recv_msg), 0);
		if(recv_size == -1)
		{
			perror("接受数据异常");
			break;
		}
		else if(recv_size == 0)//代表客户端断开连接
		{
			//删除链表节点
			break;
		}

		printf("新的消息:\n%s：%s\n", recv_msg.name, recv_msg.buffer);
	
	}

	return NULL;

}




//./client 192.168.33.3  名字
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

	struct msg_info msg;
	ssize_t send_size;
	pthread_t tid;


	pthread_create(&tid, NULL, recv_data, &skt_fd);

	strcpy(msg.name, argv[2]);

	char buffer[1024];

	while(1)
	{
		scanf("%s", buffer);

		//msg.pake_size = 20+strlen(msg.buffer);

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
