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

enum
{
	online_flag,
	offline_flag,
	msg_flag
};

struct friend_list
{
	char name[256];
	struct sockaddr_in addr;
	struct friend_list *next;
};

//消息结构体
struct recv_info
{
	char name[256];
	int msg_flag;
	char msg_buffer[4096];
};

struct glob_info
{
	char name[256];
	int skt_fd;
	struct friend_list *list_head;
};

#define list_for_each(head, pos) \
	for (pos = head->next; pos != NULL; pos = pos->next)

//申请客户端信息链表的头节点
static struct friend_list *request_friend_info_node(const struct friend_list *info)
{
	struct friend_list *new_node;

	new_node = malloc(sizeof(struct friend_list));
	if (new_node == NULL)
	{
		perror("申请客户端节点异常");
		return NULL;
	}

	if (info != NULL)
		*new_node = *info;

	new_node->next = NULL;

	return new_node;
}

static inline void insert_friend_info_node_to_link_list(struct friend_list *head, struct friend_list *insert_node)
{
	struct friend_list *pos;

	for (pos = head; pos->next != NULL; pos = pos->next)
		;

	pos->next = insert_node;
}

//自动检索本地网卡的所有信息，发送广播信息到所有网卡（除本地回环网卡）
int broadcast_msg_data(int skt_fd, const void *msg, ssize_t msg_len)
{
	int i;
	struct ifconf ifconf;
	struct ifreq *ifreq;
	struct sockaddr_in dest_addr;
	ssize_t send_size;
	char buf[512]; //缓冲区
	//初始化ifconf
	ifconf.ifc_len = 512;
	ifconf.ifc_buf = buf;

	ioctl(skt_fd, SIOCGIFCONF, &ifconf); //获取所有接口信息

	//接下来一个一个的获取IP地址
	ifreq = (struct ifreq *)ifconf.ifc_buf;

	//printf("获取到的所有网卡信息结构体长度:%d\n",ifconf.ifc_len);
	//printf("一个网卡信息结构体的差精度%ld\n", sizeof (struct ifreq));

	//循环分解每个网卡信息
	//i=(ifconf.ifc_len/sizeof (struct ifreq))等于获取到多少个网卡
	for (i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; i--, ifreq++)
	{
		if (ifreq->ifr_flags == AF_INET) //判断网卡信息是不是IPv4的配置
		{
			//printf("网卡名字叫 [%s]\n" , ifreq->ifr_name);
			//printf("网卡配置的IP地址为  [%s]\n" ,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr));

			if (strcmp(ifreq->ifr_name, "lo") == 0) //判断如果是本地回环网卡则不广播数据
				continue;

			ioctl(skt_fd, SIOCGIFBRDADDR, ifreq); //通过网卡名字获取广播地址

			//将网络地址转化为本机地址
			//printf("该网卡广播地址为 %s\n", inet_ntoa(((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr));

			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(56633);
			dest_addr.sin_addr.s_addr = ((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr.s_addr;

			send_size = sendto(skt_fd, msg, msg_len,
							   0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if (send_size == -1)
			{
				perror("发送UDP数据失败\n");
				return -1;
			}
		}
	}

	return 0;
}

void *recv_broadcast_msg(void *arg)
{
	struct recv_info recv_msg, msg_info;
	struct glob_info *ginfo = arg;
	socklen_t skt_len = sizeof(struct sockaddr_in);

	//一直接受他人的广播信息，代表好友上线，更新好友链表

	ssize_t recv_size, send_size;

	struct friend_list *new_node;
	struct friend_list cache_node;
	struct friend_list *pos;

	while (1)
	{
		bzero(&recv_msg, sizeof(recv_msg));

		recv_size = recvfrom(ginfo->skt_fd, &recv_msg, sizeof(recv_msg),
							 0, (struct sockaddr *)&(cache_node.addr), &skt_len);
		if (recv_size == -1)
		{
			perror("接受UDP数据失败\n");
			break;
		}

		printf("你的%s给你发送消息：%s\n", recv_msg.name, recv_msg.msg_buffer);

		switch (recv_msg.msg_flag)
		{
		case online_flag:
			list_for_each(ginfo->list_head, pos)
			{
				if (strcmp(pos->name, recv_msg.name) == 0)
					break;
			}

			if (pos != NULL)
				break;

			//谁谁谁上线了，插入好友链表
			strcpy(cache_node.name, recv_msg.name);
			new_node = request_friend_info_node(&cache_node);
			insert_friend_info_node_to_link_list(ginfo->list_head, new_node);
			printf("%s上线了\n", new_node->name);

			strcpy(msg_info.name, ginfo->name); //将我们的名字赋值进去
			msg_info.msg_flag = online_flag;	//上线标志

			send_size = sendto(ginfo->skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name,
							   0, (struct sockaddr *)&cache_node.addr, sizeof(struct sockaddr_in)); //回送对方我们的上线信息

			break;

		case offline_flag:
			//谁谁谁下线了，删除这个好友链表
			break;
		case msg_flag:
			//谁谁谁给你发送消息，将消息如何处理
			printf("你的%s给你发送消息：%s\n", recv_msg.name, recv_msg.msg_buffer);
			break;
		}
	}
}

//./udp 昵称
int main(int argc, const char *argv[])
{
	int udp_fd;
	int retval;
	int input_cmd;
	ssize_t send_size;
	char broadcase_addr[16];
	struct sockaddr_in native_addr, dest_addr, recv_addr;
	socklen_t skt_len = sizeof(struct sockaddr_in);
	struct glob_info ginfo;
	struct friend_list *pos;
	pthread_t tid;

	strcpy(ginfo.name, argv[1]);
	ginfo.list_head = request_friend_info_node(NULL);

	ginfo.skt_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (ginfo.skt_fd == -1)
	{
		perror("申请套接字失败");
		goto request_socket_err;
	}

	int sw = 1;

	retval = setsockopt(ginfo.skt_fd, SOL_SOCKET, SO_BROADCAST, &sw, sizeof(sw));
	if (retval == -1)
	{
		perror("设置程序允许广播出错");
		goto setsock_broadcase_err;
	}

	native_addr.sin_family = AF_INET;
	native_addr.sin_port = htons(56633);
	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	retval = bind(ginfo.skt_fd, (struct sockaddr *)&native_addr, sizeof(native_addr));
	if (retval == -1)
	{
		perror("绑定异常");
		return -1;
	}

	pthread_create(&tid, NULL, recv_broadcast_msg, &ginfo);

	//根据网卡名字获取IP地址是多少
	//get_netcard_broadcase_addr(udp_fd, "ens38", broadcase_addr, sizeof(broadcase_addr));

	//printf("获取的本地广播地址信息：broadcase_ip=%s\n", broadcase_addr);

	struct recv_info msg_info;

	strcpy(msg_info.name, argv[1]);	 //将我们的名字赋值进去
	msg_info.msg_flag = online_flag; //上线标志

	broadcast_msg_data(ginfo.skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name); //通知别人我们上线了

	while (1)
	{
		scanf("%d", &input_cmd);

		switch (input_cmd)
		{
		case 1:
			//打印好友链表，并且可以找寻一个好友聊天
			list_for_each(ginfo.list_head, pos)
			{
				printf("性感%s，在线陪聊\n", pos->name);
			}

			break;
		case 2:
			//仅仅只是打印好友链表
			break;

		case 0:
			//下线，通知其他人我们走了
			break;
		}
	}

	close(ginfo.skt_fd);

	return 0;

setsock_broadcase_err:
bind_socket_err:
	close(udp_fd);
request_socket_err:
	return -1;
}
