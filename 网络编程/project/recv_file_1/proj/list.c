#include "pro_h.h"

//申请好友信息链表的节点
struct friend_list* request_friend_info_node(const struct friend_list* info)
{
	struct friend_list* new_node;
	new_node = malloc(sizeof(struct friend_list));
	if(new_node == NULL)
	{
		perror("申请节点失败\n");
		return NULL;
	}
	
	if(info != NULL)
		*new_node = *info;
	
	new_node->next = NULL;
	
	return new_node;
	
}		

//插入信息节点
void insert_friend_info_node_to_link_list(struct friend_list *head, struct friend_list *insert_node)
{
	struct friend_list *pos;

	//插到链表末尾
	for(pos=head; pos->next != NULL; pos=pos->next);

	pos->next = insert_node;
}

//删除下线的信息节点
void remove_friend_node(struct friend_list* head, char* offline_name)
{
	struct friend_list* p;
	struct friend_list* pos;
	
	//遍历找到下线人名字，再把该节点移除
	for(p = head, pos = head->next; pos != NULL; p = pos, pos = pos->next)
	{
		if(strcmp(pos->name, offline_name) == 0)
		{
			p->next = pos->next;
			pos->next = NULL;
			free(pos);
			
			printf("当前好友列表更新为:");
			//有好友下线就刷新一下好友列表
			list_for_each(head,pos)
					printf("好友%s(%s)在线\n\n",pos->name, pos->gender);
			break;
		}
	}
}