#include "pro_h.h"

//接收文件线程
void* recv_pthread(void* arg)
{
	char file_name[256];
	char buf[1024];
	int tcp_fd;
	int retval;
	ssize_t recv_size;
	struct recv_info* recv_msg = arg;
	
	tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(tcp_fd == -1)
	{
		perror("申请tcp套接字失败\n");
		return NULL;
	}
	
	int sw=1;
	//设置地址及端口可以被复用的属性
	retval = setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &sw, sizeof(sw));
	if(retval == -1)
	{
		perror("设置地址及端口复用失败");
		return NULL;
	}
	
	struct sockaddr_in send_addr;
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(6666);
	send_addr.sin_addr.s_addr = inet_addr(recv_msg->ip);
	
	retval = connect(tcp_fd, (struct sockaddr* )&send_addr, sizeof(send_addr));
	if(retval == -1)
	{
		perror("tcp连接失败\n");
		return NULL;
	}
	printf("\ntcp连接成功,即将开始接收文件，请稍后...\n");
	
	//组合更新文件名
	sprintf(file_name, "%s%s", "recv_file_", recv_msg->file_msg.file_name);
	
	//打开创建文件准备接收数据
	int fd = open(file_name, O_RDWR|O_CREAT, 0664);
	if(fd == -1)
	{
		perror("创建新文件失败\n");
		return NULL;
	}
	
	while(1)
	{
		bzero(buf, sizeof(buf));
		recv_size = recv(tcp_fd,buf, sizeof(buf), 0);
		if(recv_size == -1)
		{
			perror("接收数据失败");
			return NULL;
		}
		//好友断开连接或接收完毕
		else if(recv_size == 0)
		{			
			break;
		}
		while(recv_size > 0)
		{
			int i = write(fd, buf, recv_size);    
			if(i == -1)
			{
				perror("写入数据失败");
				return NULL;
			}
			recv_size -= i;
		}					
	}
	printf("文件接收完毕\n");
	close(fd);          //关闭文件描述符
	close(tcp_fd);      //	
	pthread_exit(NULL);	//结束此线程
}
		
//一直接收广播信息
void* recv_broadcast_msg(void* arg)
{
	pthread_t tid1;
	ssize_t recv_size, send_size;
	struct send_info* sinfo = arg; 
	
	struct recv_info recv_msg;
	struct friend_list cache_node; 
	struct friend_list* pos;
	struct friend_list* new_node;
	socklen_t skt_len = sizeof(struct sockaddr_in);
	
	/*
		       &recv_msg:存放接收到的信息
		sizeof(recv_msg):接收信息的最大长度
		 cache_node.addr:好友地址
	*/
	while(1)
	{
		bzero(&recv_msg, sizeof(recv_msg));
		
		recv_size = recvfrom(sinfo->skt_fd, &recv_msg, sizeof(recv_msg), 0, 
							 (struct sockaddr* )&cache_node.addr, &skt_len);
		if(recv_size == -1)
		{
			perror("接收UDP数据失败\n");
			break;
		}
		
		switch(recv_msg.msg_flag)
		{
			case online_flag:
				list_for_each(sinfo->list_head, pos)
				{
					if(strcmp(pos->name, recv_msg.name) == 0)
						break; //跳出for循环遍历
				}
				
				//证明链表已有该好友（不是第一次上线），跳出switch
				if(pos != NULL)
					break;
				
				//插入好友链表
				strcpy(cache_node.name, recv_msg.name);
				strcpy(cache_node.gender, recv_msg.gender);
				strcpy(cache_node.ip, inet_ntoa(cache_node.addr.sin_addr));
				new_node = request_friend_info_node(&cache_node);
				insert_friend_info_node_to_link_list(sinfo->list_head, new_node);
				
				printf("%s(%s)上线了\n", new_node->name, new_node->gender);
				
				struct recv_info msg_info;
				strcpy(msg_info.name, sinfo->name); //将自己的名字复制到msg_info.name
				strcpy(msg_info.gender, sinfo->gender); 
				msg_info.msg_flag =online_flag;//上线标志
				
				//回送对方我们的上线信息
				send_size = sendto(sinfo->skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name,
								   0, (struct sockaddr* )&cache_node.addr, sizeof(struct sockaddr_in));
				break;
				
			case offline_flag:
				printf("%s\n已下线\n", recv_msg.name);
				//删除下线好友信息节点
				remove_friend_node(sinfo->list_head, recv_msg.name);
				break;
				
			case msg_flag:
				//接收文字消息
				printf("\n%s(%s)发来消息：%s\n\n", recv_msg.name, recv_msg.gender, recv_msg.msg_buffer);
				break;
				
			case file_flag:
				sleep(1);
				//接收文件
				pthread_create(&tid1, NULL, recv_pthread,&recv_msg);
				pthread_join(tid1, NULL);                                    
				break;
		}
	}
}