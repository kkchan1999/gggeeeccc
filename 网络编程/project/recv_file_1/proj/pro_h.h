#include <stdio.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <strings.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

//好友信息结构体
struct friend_list{
	char name[16];
	char gender[4];
	char ip[64];
	struct sockaddr_in addr;
	struct friend_list* next;
};

//发送文件结构体
struct send_file{
	char file_name[128];
	int file_type;
	ssize_t file_size;
};

//接收消息结构体
struct recv_info{
	char name[16];
	char gender[4];
	char ip[64];
	int msg_flag;
	char msg_buffer[1024];
	struct send_file file_msg;
};

//发送信息结构体
struct send_info{
	char name[16];
	char gender[4];
	int skt_fd;
	struct friend_list* list_head;
};

#define list_for_each(head, pos)\
	for(pos=head->next; pos!=NULL; pos=pos->next)

char native_ip[64];
enum {online_flag, offline_flag, msg_flag, file_flag};

//申请好友信息链表的节点
struct friend_list* request_friend_info_node(const struct friend_list* info);

//插入信息节点
void insert_friend_info_node_to_link_list(struct friend_list *head, struct friend_list *insert_node);

//删除下线的信息节点
void remove_friend_node(struct friend_list* head, char* offline_name);

//接收文件线程
void* recv_pthread(void* arg);

//一直接收广播信息
void* recv_broadcast_msg(void* arg);

//自动检索本地网卡的所有信息，发送广播信息到所有网卡（除本地回环网卡）
int broadcast_msg_data(int skt_fd, const void *msg, ssize_t msg_len);

//发送文件线程
void* send_pthread(void* arg);

//发送文件给好友
void send_file_to_friend(struct friend_list *pos, struct send_info sinfo, struct recv_info msg_info);

//与好友文字聊天
void choose_friend_to_chat(struct send_info sinfo, struct recv_info msg_info);

//下线
void offline(int skt_fd, struct recv_info msg_info);