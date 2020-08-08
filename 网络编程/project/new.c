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

//消息类型
enum
{
    online_flag,
    offline_flag,
    msg_flag,
    file_flag, //多加一个文件flag,接到这个东西之后就开一个tcp线程去准备接收
    recv_file_flag
};
//朋友链表
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
//这个是干嘛的目前不知道
struct glob_info
{
    char name[256];
    int skt_fd;
    struct friend_list *list_head;
};
//遍历链表
#define list_for_each(head, pos) \
    for (pos = head->next; pos != NULL; pos = pos->next)

//申请客户端信息链表头节点？干嘛的？
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
        *new_node = *info; //？？？可能是为了不被吃掉

    new_node->next = NULL;

    return new_node;
}

static inline void insert_friend_info_node_to_link_list(struct friend_list *head, struct friend_list *insert_node)
{
    struct friend_list *pos;

    for (pos = head; pos->next != NULL; pos = pos->next)
        ; //跑到链表最后

    pos->next = insert_node; //插到后面
}

//自动获取网卡信息，发送广播信息
int broadcast_msg_data(int skt_fd, const void *msg, ssize_t msg_len)
{
    int i;
    struct ifconf ifconf; //貌似怎么取名都会报错，可能是bug
    struct ifreq *ifreq;
    struct sockaddr_in dest_addr;
    ssize_t send_size;
    char buf[512];

    //初始化ifconf
    ifconf.ifc_len = 512;
    ifconf.ifc_buf = buf;

    ioctl(skt_fd, SIOCGIFCONF, &ifconf); //获取所有接口信息

    ifreq = (struct ifreq *)ifconf.ifc_buf;

    for (i = (ifconf.ifc_len / sizeof(struct ifreq)); i > 0; i--, ifreq++)
    {
        if (ifreq->ifr_flags == AF_INET)
        {
            if (strcmp(ifreq->ifr_name, "lo") == 0)
                continue;

            ioctl(skt_fd, SIOCGIFBRDADDR, ifreq);

            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(56653); //为什么是这个？
            dest_addr.sin_addr.s_addr = ((struct sockaddr_in *)&(ifreq->ifr_addr))->sin_addr.s_addr;

            send_size = sendto(skt_fd, msg, msg_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

            if (send_size == -1)
            {
                perror("发送UDP数据失败");
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

    ssize_t recv_size, send_size;

    struct friend_list *new_node;
    struct friend_list cache_node;
    struct friend_list *pos, *prev;

    // 一直接收别人的信息
    while (1)
    {
        bzero(&recv_msg, sizeof(recv_msg));

        recv_size = recvfrom(ginfo->skt_fd, &recv_msg, sizeof(recv_msg), 0, (struct sockaddr *)&(cache_node.addr), &skt_len);

        if (recv_size == -1)
        {
            perror("接收UDP数据失败");
            break;
        }

        switch (recv_msg.msg_flag)
        {
        case online_flag:
            list_for_each(ginfo->list_head, pos)
            {
                if (strcmp(pos->name, recv_msg.name) == 0)
                    break;
            }
            if (pos != NULL)
                break; //如果这个家伙已经在自己的链表里面,就不用加了

            //插入链表
            strcpy(cache_node.name, recv_msg.name);
            new_node = request_friend_info_node(&cache_node);
            insert_friend_info_node_to_link_list(ginfo->list_head, new_node);
            printf("%s上线啦\n", new_node->name);

            strcpy(msg_info.name, ginfo->name);
            msg_info.msg_flag = online_flag;

            send_size = sendto(ginfo->skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name, 0, (struct sockaddr *)&cache_node.addr, sizeof(struct sockaddr_in)); //回复自己上线的信息.

            break;

        case offline_flag:
            //谁谁谁下线了，删除这个好友链表
            for (pos = ginfo->list_head->next, prev = ginfo->list_head; pos != NULL; prev = pos, pos = pos->next)
            {
                if (strcmp(pos->name, recv_msg.name) == 0)
                {
                    //找到下线这个b了
                    prev->next = pos->next; //上一个节点指向pos的next
                    printf("%s下线了,", pos->name);
                    free(pos);
                    printf("删除节点成功！\n");
                    break;
                }
            }
            break;

        case msg_flag:
            printf("%s发送消息:%s\n", recv_msg.name, recv_msg.msg_buffer);
            break;

        case file_flag:
            //收到别人发送文件的请求
            break;

        case recv_file_flag:
            //别人准备好了接收之后
            break;

        default:
            break;
        }
    }
}

void *send_file(void *arg)
{
}

void *recv_file(void *arg)
{
}

int main(int argc, char const *argv[])
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
    pthread_t tid; //接收用的

    strcpy(ginfo.name, argv[1]);
    ginfo.list_head = request_friend_info_node(NULL);

    ginfo.skt_fd = socket(AF_INET, SOCK_DGRAM, 0);
    ginfo.skt_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ginfo.skt_fd == -1)
    {
        perror("申请套接字失败");
        goto request_socket_err;
    }

    int sw = 1;
    retval = setsockopt(ginfo.skt_fd, SOL_SOCKET, SO_BROADCAST, &sw, sizeof(sw)); //sw那个位置只要不是NULL就行了
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

    struct recv_info msg_info;

    strcpy(msg_info.name, argv[1]);  //将我们的名字赋值进去
    msg_info.msg_flag = online_flag; //上线标志

    broadcast_msg_data(ginfo.skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name); //通知别人我们上线了

    bool exit_flag = false;
    while (!exit_flag)
    {
        int friend_num;
        scanf("%d", &input_cmd);

        switch (input_cmd)
        {
        case 1:
            friend_num = 0;
            list_for_each(ginfo.list_head, pos)
            {
                friend_num++;
            }

            if (friend_num == 0)
            {
                printf("好友链表中没有好友\n");
                break;
            }

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
            msg_info.msg_flag = offline_flag; //下线标志
            broadcast_msg_data(ginfo.skt_fd, &msg_info, msg_info.msg_buffer - msg_info.name);
            exit_flag = true;
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
