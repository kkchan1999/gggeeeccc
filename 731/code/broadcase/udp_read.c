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

//./udp 6666
int main(int argc, const char *argv[])
{
	char buffer[1024];
	int udp_fd;
	int retval;
	ssize_t recv_size;
	struct sockaddr_in native_addr, recv_addr;
	socklen_t skt_len = sizeof(struct sockaddr_in);

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

		printf("读取到%ld字节的数据：%s\n", recv_size, buffer);
		
	}

	close(udp_fd);


	return 0;

bind_socket_err:
	close(udp_fd);
request_socket_err:
	return -1;

}

