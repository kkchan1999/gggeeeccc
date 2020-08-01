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
#include <unistd.h>
#include <stdlib.h>       


typedef struct client_info{
	int client_fd;
	char name[256];
	char ip[16];
	short port;
	pthread_t tid;

	struct client_info *next;
}client_info_node;

#define	list_for_each(head, pos)\
	for(pos=head->next; pos!=NULL; pos=pos->next)

//申请客户端信息链表的头节点
static client_info_node *request_client_info_node(const client_info_node *info)
{
	client_info_node *new_node;

	new_node = malloc(sizeof(client_info_node));
	if(new_node == NULL)
	{
		perror("申请客户端节点异常");
		return NULL;
	}

	if(info != NULL)
		*new_node = *info;

	new_node->next = NULL;

	return 	new_node;
}

static inline void insert_client_info_node_to_link_list(client_info_node *head, client_info_node *insert_node)
{
	client_info_node *pos;

	for(pos=head; pos->next != NULL; pos=pos->next);

	pos->next = insert_node;
}


int main(void)
{
	char buffer[1024];
	int skt_fd;
	int retval;
	int client_fd;
	int max_fd;
	ssize_t recv_size, send_size;

	fd_set readfds;
	socklen_t skt_len = sizeof(struct sockaddr_in);
	
	struct sockaddr_in client_addr, native_addr;
	
	client_info_node cache_client_info;
	client_info_node *list_head, *new_node, *pos, *send_pos, *rm_prev;
	



	list_head = request_client_info_node(NULL);

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

	

	native_addr.sin_family = AF_INET;//指定引用IPV4的协议

	native_addr.sin_port = htons(6666);//指定端口号，转化为网络字节序（大端序）
	
	//inet_aton(INADDR_ANY, &(native_addr.sin_addr));

	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);//将所有的IP地址转化为二进制的网络字节序的数据进行绑定

	

	/*
		将指定的地址信息及本程序的套接字绑定在一起
		skt_fd：套接字的文件描述符
		&native_addr：需要绑定的地址信息结构体（每个协议的地址信息结构体是不一样的，我们用IPV4的协议便需要引用struct sockaddr_in）
		sizeof(native_addr)：传入的结构体长度
	*/
	retval = bind( skt_fd, (struct sockaddr *)&native_addr, sizeof(native_addr));
	if(retval == -1)
	{
		perror("绑定套接字地址失败");
		goto bind_socket_err;
	}


	/*
		设置套接字的同时通信最大连接数为50，并且将这个套接字的属性设置为可监听属性
	*/
	retval = listen(skt_fd, 50);
	if(retval == -1)
	{
		perror("设置最大连接数失败");
		goto set_listen_err;
	}
	
	/*
		void FD_CLR(int fd, fd_set *set);//将指定的文件描述符清除出集合
		int  FD_ISSET(int fd, fd_set *set);//判断指定的文件描述符是否在集合当中
		void FD_SET(int fd, fd_set *set);//将指定的文件描述符添加到集合当中
		void FD_ZERO(fd_set *set);	//将整个集合当中的文件描述符清除掉
	*/

	max_fd = skt_fd;

	FD_ZERO(&readfds);
	

	while(1)
	{
		FD_SET(0, &readfds);//将标准输入的文件描述符放到集合里面
		FD_SET(skt_fd, &readfds);//网络链接的文件描述符放到集合里面

		list_for_each(list_head, pos)
		{
			FD_SET(pos->client_fd, &readfds);
		}

		
		/*
			多路复用：一次性监控多个文件描述符
			int select(int nfds, fd_set *readfds, fd_set *writefds,
		          fd_set *exceptfds, struct timeval *timeout);
			nfds：要监控的文件描述符中最大的那个值+1
			readfds：读文件描述符集合
			writefds：写文件描述符集合
			exceptfds：其他类文件描述符集合	
			timeout：规定我只在这里阻塞多长的时间，如果时间一到还没有文件描述符有反应我们select函数也不等待了，直接返回0
		*/
		retval = select(max_fd+1, &readfds, NULL, NULL, NULL);
		if(retval == -1)
		{
			perror("复用等待异常");
			goto select_err;
		}
		else if(retval == 0)
		{
			printf("服用等待超时\n");
		}else if(retval)//如果真的有文件描述符相应，则进入这个判断
		{
			list_for_each(list_head, pos)
			{
				if(FD_ISSET(pos->client_fd, &readfds))
				{
					bzero(buffer, sizeof(buffer));
					recv_size = recv(pos->client_fd, buffer, sizeof(buffer), 0);
					if(recv_size == -1)
					{
						perror("读取数据异常");
						continue;
					}else if(recv_size == 0)
					{
						printf("客户端断开链接\n");
						//删除链表节点
						for(rm_prev=list_head; rm_prev->next != pos && rm_prev != NULL; rm_prev=rm_prev->next);

						rm_prev->next = pos->next;
						free(pos);

						break;
					}else{
			
						list_for_each(list_head, send_pos)
						{
							if(send_pos->client_fd == pos->client_fd)
								continue;

							printf("转发数据给端口号为%hu\n", pos->port);

							send_size = send( send_pos->client_fd, buffer, recv_size, 0);
							if(send_size == -1)
								break;
						}
						printf("接收到来自与客户端%ld个字节的数据：%s\n", recv_size, buffer);
					}
				}
			
			}

			if(FD_ISSET(0, &readfds))//如果0号文件描述符还在集合中，便证明我们当前的相应便是标准输入发起的
			{
				scanf("%s", buffer);
				printf("从键盘当中获取到：%s\n", buffer);
			}

			if(FD_ISSET(skt_fd, &readfds))//如果skt_fd在集合中，则证明是有客户端链接服务器，我们才相应了
			{
				
				cache_client_info.client_fd = accept(skt_fd, (struct sockaddr *)&client_addr, &skt_len);
				if(cache_client_info.client_fd == -1)
				{
					perror("客户端链接失败");
					goto client_connect_err;
				}

				max_fd = max_fd>cache_client_info.client_fd? max_fd: cache_client_info.client_fd;

				strcpy(cache_client_info.ip, inet_ntoa(client_addr.sin_addr));//存放IP地址
				cache_client_info.port = ntohs(client_addr.sin_port);

				//新建节点
				new_node = request_client_info_node(&cache_client_info);
				//将节点插入链表
				insert_client_info_node_to_link_list(list_head, new_node);

				printf("服务器：客户端连接成功\n");
				printf("客户端信息：\n客户端IP为%s，端口号为%hu\n", cache_client_info.ip, cache_client_info.port);
			}
		}
			
	}

	return 0;




client_connect_err:

select_err:
set_listen_err:
bind_socket_err:
	close(skt_fd);
	return -1;
}
