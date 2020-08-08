#include "pro_h.h"

//发送文件线程（TCP协议）
void* send_pthread(void* arg)
{
	char buf[1024];  //读取缓冲区
	int tcp_fd;
	int retval;
	int recv_fd;
	ssize_t send_size;
	struct send_file* packet = arg;
	
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
	
	//结构体变量(本地地址)
	struct sockaddr_in native_addr;
	//IPV4协议
	native_addr.sin_family = AF_INET;
	//端口号，转为网络字节序（大端序）
	native_addr.sin_port = htons(6666);
	//绑定所有IP地址，并转化为网络字节序
	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	retval = bind(tcp_fd, (struct sockaddr* )&native_addr, sizeof(native_addr));
	if(retval == -1)
	{
		perror("绑定tcp套接字地址失败\n");
		close(tcp_fd);
		return NULL;
	}
	
	/*
		设置套接字的同时通信最大连接数为30
		并且将套接字的属性设置为可监听属性
	*/
	retval = listen(tcp_fd, 30);
	if(retval == -1)
	{
		perror("设置最大连接数失败\n");
		close(tcp_fd);
		return NULL;
	}
	
	//客户端地址信息结构体
	struct sockaddr_in recv_addr;
	//结构体长度
	socklen_t addrlen = sizeof(recv_addr);
	
	/*
		等待客户端链接，连接成功返回一个代表客户端通信的文件描述符，具备阻塞特性
			 tcp_fd: 代表套接字接口
		client_addr：存放客户端地址信息
			addrlen: 结构体长度
	*/
	recv_fd = accept(tcp_fd, (struct sockaddr*) &recv_addr, &addrlen);
	if(recv_fd == -1)
	{
		perror("客户端连接失败\n");
		close(tcp_fd);
		return NULL;
	}
	printf("\n好友连接成功\n");
	printf("好友信息:\nIP:%s, 端口号:%hu\n", 
			inet_ntoa(recv_addr.sin_addr), ntohs(recv_addr.sin_port));
	printf("文件正在传输中，请稍等...\n");
			
	//打开要传输的文件
	int fd = open(packet->file_name, O_RDONLY);	
	if(fd == -1)
	{
		perror("打开文件失败\n");
		return NULL;
	}
	
	//读取一次发送一次，循环直达文件读取完毕
	while(1)
	{
		bzero(buf, sizeof(buf));
		int size = read(fd, buf, sizeof(buf));
		if(size == 0)
		{
			printf("发送完毕\n");
			break;
		}
		if(size == -1)
		{
			perror("读取文件数据失败\n");
			break;
		}
		send_size = write(recv_fd, buf, size);
		if(send_size == -1)
		{
			perror("发送文件数据失败\n");
			break;
		}
	}
	sleep(2);
	close(recv_fd);
	close(tcp_fd);
	close(fd);
	pthread_exit(NULL);
}

//发送文件给好友
void send_file_to_friend(struct friend_list *pos, struct send_info sinfo, struct recv_info msg_info)
{
	char buffer[128];
	ssize_t send_size;
	//struct friend_list* pos;

	printf("请输入要发送的文件名：");
	scanf("%s", buffer);
	
	struct stat info; //存放文件属性
	stat(buffer, &info);
	
	struct send_file packet;
	strcpy(packet.file_name, buffer); //复制文件名
	packet.file_type = info.st_mode;  //文件类型、权限
	packet.file_size = info.st_size;  //文件大小
	
	msg_info.file_msg = packet;
	
	stpcpy(msg_info.ip, native_ip);   //ip给好友
	
	// 设置为文件信息
	msg_info.msg_flag = file_flag;
	
	printf("即将发送文件名：%s ,大小：%ld字节\n",msg_info.file_msg.file_name, msg_info.file_msg.file_size);
			
	//发送文件信息给好友
	send_size = sendto(sinfo.skt_fd, &msg_info, sizeof(msg_info), 0,
						(struct sockaddr* )&pos->addr, sizeof(struct sockaddr_in));				   
	if(send_size == -1)
	{
		perror("信息发送失败\n");
		return;
	}
	
	//tcp发送文件线程
	pthread_t tid;
	pthread_create(&tid, NULL, send_pthread, &packet);  
	pthread_join(tid, NULL);          
}

//与好友文字聊天
void choose_friend_to_chat(struct send_info sinfo, struct recv_info msg_info)
{
	char name[16];    //写入选择聊天好友名字
	int num;          //选择通信方式（文字聊天or传输文件or返回上一层）
	ssize_t send_size;
	struct friend_list *pos;
	
input_name:
	printf("========================\n");
	printf("<请输入好友名字进行聊天>\n");
	list_for_each(sinfo.list_head, pos)
		printf("好友%s(%s)在线\n", pos->name, pos->gender);
	printf("========================\n");
	printf("返回上一层输入exit\n");
	
	printf("请输入:");
	scanf("%s", name);
	if(strcmp(name, "exit") == 0)
		return;
	
	else
	{
		list_for_each(sinfo.list_head, pos)
		{
			if(strcmp(name, pos->name) == 0)
			{	
				while(1)
				{
					printf("\n/////////////////\n");
					printf("//发送信息输入1//\n//发送文件输入2//\n//返回上层输入3//\n");
					printf("/////////////////\n");
					printf("请输入:");
					scanf("%d", &num);
					switch(num)
					{
						//发送文字信息
						case 1:
							printf("\n请开始聊天，返回上一层输入exit\n");
							while(1)
							{
								printf("请输入:");
								scanf("%s", msg_info.msg_buffer);
								if(strcmp(msg_info.msg_buffer, "exit") == 0)
								{
									bzero(msg_info.msg_buffer, sizeof(msg_info));
									break;
								}
								
								// 设置为普通信息
								msg_info.msg_flag = msg_flag;
								send_size = sendto(sinfo.skt_fd, &msg_info, sizeof(msg_info), 0,
												   (struct sockaddr* )&pos->addr, sizeof(struct sockaddr_in));				   
								if(send_size == -1)
								{
									perror("发送信息失败\n");
									break;
								}
								sleep(1); 
								printf("可继续聊天，输入exit则退出聊天页面\n");
							}
							break;
						
						//发送文件
						case 2:
							//发送文件
							send_file_to_friend(pos, sinfo, msg_info);
							break;
							
						//返回上层选择好友聊天
						case 3:
							printf("\n");
							goto input_name;
							
						default:
							printf("请输入正确的数字\n");
					}
				}
			}
		}
		printf("\n无法识别该好友%s，请重新输入\n", name);
		goto input_name;
	}
}