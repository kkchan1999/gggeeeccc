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
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>


int get_netcard_broadcase_addr(int skt_fd, char *netcard_name, char *ip_addr, int ip_len)
{
	int retval;
	struct ifreq ifr;
	struct sockaddr_in get_addr, cache_addr;
	unsigned int addr_numb;

	bzero(&ifr, sizeof(ifr));//内存清空
		
	strcpy(ifr.ifr_name, netcard_name);//将网卡名字放入到ifr.ifr_name的内存当中，指定好网卡的名字

	retval = ioctl(skt_fd, SIOCGIFADDR, &ifr);//直接获取指定网卡的地址信息到ifr结构体里面
	if(retval != 0)
	{
		perror("获取指定网卡IP地址失败");
		return -1;
	}

	memcpy(&get_addr, &(ifr.ifr_addr), sizeof(get_addr));//将地址信息拷贝到get_addr结构体里面拿来分析

	addr_numb = ntohl(get_addr.sin_addr.s_addr);//将网络字节数的二进制IP地址转化为本地字节序的二进制IP地址，方便我们下面做IP地址类型的判断

	printf("本机ip地址：%s\n", inet_ntoa(get_addr.sin_addr));//将获取到的网卡IP地址打印出来

	if((addr_numb & 0xe0000000) <= 0x60000000)//保留32位IP地址的前三位数据，并且判断，A类地址由于是0开头，所以保留前面3位的最大值是011和面都是0，十六进制数就是0x60000000
	{
			cache_addr.sin_addr.s_addr = htonl(addr_numb|0x00ffffff);//如果他是A类地址，他的网络地址则是前8位二进制，剩下的24位都是主机地址，全部置1便是广播地址（255就是全部都是1），并且转化为网络字节序存放进去变量当中

			printf("这个是A类地址，广播地址为%s\n", inet_ntoa(cache_addr.sin_addr));//将广播地址打印出来
	}
	else if((addr_numb & 0xe0000000) <= 0xa0000000)//同理，B类地址10开头，保留3位则是101是最大值，所以十六进制是0xa0000000
	{
			cache_addr.sin_addr.s_addr = htonl(addr_numb|0x0000ffff);

			printf("这个是B类地址，广播地址为%s\n", inet_ntoa(cache_addr.sin_addr));
	}
	else if((addr_numb & 0xe0000000) <= 0xc0000000)//同理，C类地址110开头，保留3位则是110是最大值，所以十六进制是0xc0000000
	{
			cache_addr.sin_addr.s_addr = htonl(addr_numb|0x000000ff);

			printf("这个是C类地址，广播地址为%s\n", inet_ntoa(cache_addr.sin_addr));
	}
	else
		printf("这个是D类地址（组播地址）");

	strncpy(ip_addr, inet_ntoa(cache_addr.sin_addr), ip_len);

	return 0;
}


//自动检索本地网卡的所有信息，发送广播信息到所有网卡（除本地回环网卡）
int broadcast_msg_data(int skt_fd, const void *msg, ssize_t msg_len)
{
	int i;
	struct ifconf ifconf;
	struct ifreq *ifreq;
	struct sockaddr_in dest_addr;
	ssize_t send_size;
	char buf[512];//缓冲区
	//初始化ifconf
	ifconf.ifc_len =512;
	ifconf.ifc_buf = buf;
   
	ioctl(skt_fd, SIOCGIFCONF, &ifconf); //获取所有接口信息

	//接下来一个一个的获取IP地址
	ifreq = (struct ifreq*)ifconf.ifc_buf;

	printf("获取到的所有网卡信息结构体长度:%d\n",ifconf.ifc_len);
	printf("一个网卡信息结构体的差精度%ld\n", sizeof (struct ifreq));

	//循环分解每个网卡信息
	//i=(ifconf.ifc_len/sizeof (struct ifreq))等于获取到多少个网卡
	for (i=(ifconf.ifc_len/sizeof (struct ifreq)); i>0; i--)
	{
		if(ifreq->ifr_flags == AF_INET)//判断网卡信息是不是IPv4的配置
		{
			printf("网卡名字叫 [%s]\n" , ifreq->ifr_name);
			printf("网卡配置的IP地址为  [%s]\n" ,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));

			ioctl(skt_fd, SIOCGIFBRDADDR, ifreq);//通过网卡名字获取广播地址

			//将网络地址转化为本机地址
			printf("该网卡广播地址为 %s\n", inet_ntoa(((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr));

			ifreq++;//地址偏移到下一个块网卡信息的内存地址当中

			if(strcmp(ifreq->ifr_name, "lo") == 0)//判断如果是本地回环网卡则不广播数据
				continue;

			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(6666);
			dest_addr.sin_addr.s_addr = ((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr.s_addr;

			send_size = sendto(skt_fd, msg, msg_len, 
					0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if(send_size == -1)
			{
				perror("发送UDP数据失败\n");
				return -1;
			}

		}
        }
    	
	return 0;
}

//./udp
int main(int argc, const char *argv[])
{
	char buffer[1024] = "我来啦";
	int udp_fd;
	int retval;
	ssize_t send_size;
	char broadcase_addr[16];
	struct sockaddr_in native_addr, dest_addr, recv_addr;
	socklen_t skt_len = sizeof(struct sockaddr_in);
	

	udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(udp_fd == -1)
	{
		perror("申请套接字失败");
		goto request_socket_err;
	}
	
	int sw = 1;

	retval = setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &sw, sizeof(sw));
	if(retval == -1)
	{
		perror("设置程序允许广播出错");
		goto setsock_broadcase_err;
	}

	native_addr.sin_family = AF_INET;
	native_addr.sin_port = htons(6665);
	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	retval = bind(udp_fd, (struct sockaddr *)&native_addr, sizeof(native_addr));
	if(retval == -1)
	{
		perror("绑定异常");
		return -1;
	}

	//根据网卡名字获取IP地址是多少
	//get_netcard_broadcase_addr(udp_fd, "ens38", broadcase_addr, sizeof(broadcase_addr));

	//printf("获取的本地广播地址信息：broadcase_ip=%s\n", broadcase_addr);


	broadcast_msg_data(udp_fd, "hello", 5);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(6666);
	dest_addr.sin_addr.s_addr = inet_addr(broadcase_addr);

	
	
	send_size = sendto(udp_fd, buffer, strlen(buffer), 
		0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
	if(send_size == -1)
	{
		perror("发送UDP数据失败\n");
		return -1;
	}


	ssize_t recv_size;

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

setsock_broadcase_err:
bind_socket_err:
	close(udp_fd);
request_socket_err:
	return -1;

}