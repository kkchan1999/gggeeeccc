#include "pro_h.h"

//自动检索本地网卡的所有信息，发送广播信息到所有网卡（除本地回环网卡）
int broadcast_msg_data(int skt_fd, const void *msg, ssize_t msg_len)
{		
	char buf[512];//缓冲区
	int i;
	ssize_t send_size;

	//初始化ifconf
	struct ifconf ifconf;
	ifconf.ifc_len =512;
	ifconf.ifc_buf = buf;
	
	ioctl(skt_fd, SIOCGIFCONF, &ifconf); //获取所有接口信息
	
	struct ifreq *ifreq;
	//一个一个的获取IP地址
	ifreq = (struct ifreq*)ifconf.ifc_buf;
	
	//循环分解每个网卡信息
	for (i = (ifconf.ifc_len / sizeof (struct ifreq)); i>0; i--, ifreq++)
	{
		if(ifreq->ifr_flags == AF_INET)//判断网卡信息是不是IPv4的配置
		{
			//判断如果是本地回环网卡则不广播数据
			if(strcmp(ifreq->ifr_name, "lo") == 0)
				continue;
			//通过网卡名字获取广播地址
			ioctl(skt_fd, SIOCGIFBRDADDR, ifreq);
			
			struct sockaddr_in dest_addr;
			dest_addr.sin_family = AF_INET;
			dest_addr.sin_port = htons(6666);
			dest_addr.sin_addr.s_addr = ((struct sockaddr_in* )&(ifreq->ifr_addr))->sin_addr.s_addr;
			
			send_size = sendto(skt_fd, msg, msg_len, 0, 
							   (struct sockaddr* )&dest_addr, sizeof(dest_addr));
			if(send_size == -1)
			{
				perror("发送UDP数据失败\n");
				return -1;
			}
		}
	}
	return 0;
}

//下线
void offline(int skt_fd, struct recv_info msg_info)
{  
	msg_info.msg_flag = offline_flag; //下线标志
	//广播告诉他人我们下线
	broadcast_msg_data(skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name);
	
	close(skt_fd);
	printf("已下线\n");
	
}

//./main name gender
int main(int argc, const char* argv[])
{
	int num;      //选择聊天或者下线标记
	int sw = 1;   //设置允许广播时欲设值
	int retval;   //返回值
	pthread_t tid;//接收广播信息线程ID
	struct send_info sinfo;
	struct friend_list *pos;
	
	strcpy(sinfo.name, argv[1]);
	strcpy(sinfo.gender, argv[2]);
	
	//申请头节点
	sinfo.list_head = request_friend_info_node(NULL);
	
	/*
		获取程序通信的套接字（接口）
		   AF_INET: IPV4协议
		SOCK_DGRAM: UDP协议
		         0：代表不改变协议内部
	*/
	sinfo.skt_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sinfo.skt_fd == -1)
	{
		perror("申请套接字失败\n");
		return -1;
	}
	
	/*
		设置socket状态
		  SOL_SOCKET:以存取socket层
		SO_BROADCAST:使用广播方式传送
		          sw:欲设置的值
	*/
	retval = setsockopt(sinfo.skt_fd, SOL_SOCKET, SO_BROADCAST, &sw, sizeof(sw));
	if(retval == -1)
	{
		perror("设置程序允许广播失败\n");
		close(sinfo.skt_fd);
		return -1;
	}
	
	//结构体变量(本地地址)
	struct sockaddr_in native_addr;
	//IPV4协议
	native_addr.sin_family = AF_INET;
	//端口号，转为网络字节序（大端序）
	native_addr.sin_port = htons(6666);
	//绑定所有IP地址，并转化为网络字节序
	native_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/*
		将指定地址信息及本程序的套接字绑定在一起
		       sinfo.skt_fd:套接字的文件描述符
		       &native_addr:需要绑定的地址信息结构体
		sizeof(native_addr):传入的结构体长度
	*/
	retval = bind(sinfo.skt_fd, (struct sockaddr* )&native_addr, sizeof(native_addr));
	if(retval == -1)
	{
		perror("绑定套接字地址失败\n");
		close(sinfo.skt_fd);
		return -1;
	}
	
	//接收广播信息
	pthread_create(&tid, NULL, recv_broadcast_msg, &sinfo);
	
	struct recv_info msg_info;
	strcpy(msg_info.name, argv[1]);  //将自己的名字复制到msg_info.name
	strcpy(msg_info.gender, argv[2]);//将自己的性别复制到msg_info.gender
	msg_info.msg_flag = online_flag; //上线标志
	
	//通知别人我们上线了
	broadcast_msg_data(sinfo.skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name);
	
	sleep(1);
	//遍历找到自己名字，把ip复制进去native_ip
	list_for_each(sinfo.list_head,pos)
	{
		if(strcmp(pos->name, argv[1])==0)
			stpcpy(native_ip, pos->ip);
	}
	
	while(1)
	{
		printf("*****************\n");
		printf("*好友聊天请输入1*\n*关机下线请输入2*\n");
		printf("*****************\n");
		scanf("%d", &num);
		switch(num)
		{
			case 1:
				//选择好友聊天
				choose_friend_to_chat(sinfo, msg_info);
				break;
				
			case 2:
				//下线
				offline(sinfo.skt_fd, msg_info);
				return 0;
				
			default:
				printf("请输入正确的数字\n");
		}
	}
}
