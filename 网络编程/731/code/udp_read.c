#include <stdio.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void *recv_thread(void *arg)
{
	int udp_fd = *(int *)arg;
	char buffer[1024];
	ssize_t recv_size;
	struct sockaddr_in recv_addr;
	socklen_t skt_len = sizeof(struct sockaddr_in);

	while(1)
	{
		bzero(buffer, sizeof(buffer));

		recv_size = recvfrom(udp_fd, buffer, sizeof(buffer), 
			0, (struct sockaddr *)&recv_addr, &skt_len);
		if(recv_size == -1)
		{
			perror("接受UDP数据失败\n");
			break;
		}

		printf("来自于IP为%s，端口号为%hu，读取到%ld字节的数据：%s\n", inet_ntoa(recv_addr.sin_addr), ntohs(recv_addr.sin_port), recv_size, buffer);
		
	}

	return NULL;
}


//./udp 自己的端口 对方的IP地址 对方的端口号
int main(int argc, const char *argv[])
{
	char buffer[1024];
	int udp_fd;
	int retval;
	ssize_t send_size;
	struct sockaddr_in dest_addr, native_addr;
	pthread_t tid;
	

	udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_fd == -1)
	{
		perror("申请套接字失败");
		goto request_socket_err;
	}

	native_addr.sin_family = AF_INET;
	native_addr.sin_port = htons(atoi(argv[1]));
	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	retval = bind(udp_fd, (struct sockaddr *)&native_addr, sizeof(native_addr));
	if(retval == -1)
	{
		perror("绑定对应的ip地址及端口失败");
		goto bind_socket_err;
	}

	pthread_create(&tid, NULL, recv_thread, &udp_fd);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(atoi(argv[3]));
	dest_addr.sin_addr.s_addr = inet_addr(argv[2]);

	

	while(1)
	{
		scanf("%s", buffer);

		send_size = sendto(udp_fd, buffer, sizeof(buffer), 
			0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if(send_size == -1)
		{
			perror("发送UDP数据失败\n");
			break;
		}

		printf("读取到%ld字节的数据：%s\n", send_size, buffer);
		
	}
	

	close(udp_fd);


	return 0;

bind_socket_err:
	close(udp_fd);
request_socket_err:
	return -1;

}
