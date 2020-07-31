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

//./udp 192.168.33.3 6666
int main(int argc, const char *argv[])
{
	char buffer[1024];
	int udp_fd;
	int retval;
	ssize_t send_size;
	struct sockaddr_in dest_addr, recv_addr;
	socklen_t skt_len = sizeof(struct sockaddr_in);

	udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_fd == -1)
	{
		perror("申请套接字失败");
		goto request_socket_err;
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(atoi(argv[2]));
	dest_addr.sin_addr.s_addr = inet_addr(argv[1]);

	

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
