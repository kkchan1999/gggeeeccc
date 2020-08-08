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
    file_flag //多加一个文件flag,接到这个东西之后就开一个tcp线程去准备接收
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
            dest_addr.sin_port = htons(56633); //为什么是这个？
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

void *recv_file(void *arg)
{
    int sockfd;
    int retval;

    char temp[512];

    strcpy(temp, (char *)arg);
    printf("temp:%s\n", temp);

    //绑定一些信息
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //ipv4 tcp
    if (sockfd == -1)
    {
        perror("申请套接字失败");
        pthread_exit(NULL);
    }

    struct sockaddr_in native_addr;

    native_addr.sin_family = AF_INET;                //指定引用IPV4的协议
    native_addr.sin_port = htons(6666);              //指定端口号，转化为网络字节序（大端序）
    native_addr.sin_addr.s_addr = htonl(INADDR_ANY); //将所有的IP地址转化为二进制的网络字节序的数据进行绑定

    retval = bind(sockfd, (struct sockaddr *)&native_addr, sizeof(native_addr));
    if (retval == -1)
    {
        perror("绑定套接字地址失败");
        pthread_exit(NULL);
    }

    retval = listen(sockfd, 1); //最多一个
    if (retval == -1)
    {
        perror("设置最大连接数失败");
        pthread_exit(NULL);
    }

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t sklen = sizeof(client_addr);

    printf("开始等待连接\n");

    client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sklen); //阻塞等待连接
    if (client_fd == -1)
    {
        perror("客户端链接失败");
        pthread_exit(NULL);
    }
    printf("服务器：客户端连接成功\n");

    char buf[256];

    //先创建一个文件
    printf("buf:%s\n", (char *)arg);
    FILE *f = fopen((char *)arg, "w+"); //这里有点问题
    int count = 0;
    while (1)
    {
        bzero(buf, sizeof(buf));
        retval = recv(client_fd, buf, 256, 0);
        if (retval == -1)
        {
            perror("读取错误!");
        }
        else if (retval == 0)
        {
            printf("连接断开了.\n");
            break;
        }
        else
        {
            fwrite(buf, 1, retval, f);
            count += retval;
        }
    }
    printf("写完了?%d\n", count);
    fclose(f);
    close(client_fd); //关闭客户端通信
    close(sockfd);    //关闭服务器的socket资源

    pthread_exit(NULL);
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

    pthread_t recv_tid, send_tid;
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
        char temp[256];
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
            //需要什么参数呢?文件名?对面的信息需要知道吗?

            strcpy(temp, recv_msg.msg_buffer);
            printf("buf:%s\n", recv_msg.msg_buffer);
            pthread_create(&recv_tid, NULL, recv_file, (void *)temp);

            //开启一个服务器,准备接收文件,准备完成后给对方发送recv_file_flag的包表明自己准备好了

            break;

        default:
            break;
        }
    }
}

int main(int argc, char const *argv[])
{
    int udp_fd;
    int retval;
    char input_cmd[256];
    ssize_t send_size;
    char broadcase_addr[16];
    struct sockaddr_in native_addr, dest_addr, recv_addr;
    socklen_t skt_len = sizeof(struct sockaddr_in);
    struct glob_info ginfo;
    struct friend_list *pos;
    pthread_t tid; //接收用的
    char buf[1024];

    strcpy(ginfo.name, argv[1]);
    ginfo.list_head = request_friend_info_node(NULL);

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
        printf("---------请输入选项---------\n");
        printf("---------1.私聊------------\n");
        printf("---------2.查看好友---------\n");
        printf("---------3.发送文件---------\n");
        printf("---------4.群发信息---------\n");
        printf("---------0.退出程序---------\n");

        int friend_num;
        scanf("%s", input_cmd);

        switch (atoi(input_cmd))
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
            else
            {
                printf("有以下%d位好友\n", friend_num);
            }
            //打印好友链表，并且可以找寻一个好友聊天
            list_for_each(ginfo.list_head, pos)
            {
                printf("%s\n", pos->name);
            }

            printf("请输入好友姓名：");
            scanf("%s", buf);
            int send_flag = 0;
            list_for_each(ginfo.list_head, pos)
            {
                if (strcmp(buf, pos->name) == 0)
                {
                    printf("请输入要发送的信息：\n");
                    scanf("%s", buf);

                    strcpy(msg_info.msg_buffer, buf);
                    msg_info.msg_flag = msg_flag;

                    send_size = sendto(ginfo.skt_fd, &msg_info, sizeof(msg_info), 0, (struct sockaddr *)&pos->addr, sizeof(pos->addr));

                    send_flag = 1;
                }
            }
            if (send_flag != 1)
            {
                printf("发送失败！");
            }
            break;

        case 2:
            //仅仅只是打印好友链表
            list_for_each(ginfo.list_head, pos)
            {

                printf("%s\n", pos->name);
            }
            break;
        case 3:
            //发送文件
            bzero(buf, sizeof(buf));
            FILE *f;
            struct sockaddr_in srv_addr;
            list_for_each(ginfo.list_head, pos)
            {
                printf("%s\n", pos->name);
            }

            printf("请输入好友姓名：");
            scanf("%s", buf);
            send_flag = 0;
            list_for_each(ginfo.list_head, pos)
            {
                if (strcmp(buf, pos->name) == 0)
                {
                    srv_addr.sin_family = AF_INET;
                    srv_addr.sin_port = htons(6666);
                    srv_addr.sin_addr.s_addr = pos->addr.sin_addr.s_addr;
                    printf("请输入文件名:");
                    scanf("%s", buf);
                    f = fopen(buf, "r");
                    if (f == NULL)
                    {
                        printf("打开文件失败!请检查是否正确填写文件名\n");
                        break;
                    }
                    bzero(msg_info.msg_buffer, sizeof(msg_info.msg_buffer));
                    strcpy(msg_info.msg_buffer, buf);
                    msg_info.msg_flag = file_flag;

                    send_size = sendto(ginfo.skt_fd, &msg_info, sizeof(msg_info), 0, (struct sockaddr *)&pos->addr, sizeof(pos->addr));

                    send_flag = 1;
                }
            }
            if (send_flag != 1)
            {
                printf("发送失败！");
            }
            sleep(2);
            int skt_fd;
            skt_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (skt_fd == -1)
            {
                perror("申请套接字失败");
                break;
            }
            retval = connect(skt_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
            if (retval == -1)
            {
                perror("客户端连接到服务器失败\n");
                break;
            }
            printf("开始发送!\n");
            int count = 0;
            while (1)
            {
                bzero(buf, sizeof(buf));
                retval = fread(buf, 1, 256, f);
                if (retval == 0)
                {
                    break;
                }
                if (retval != send(skt_fd, buf, retval, 0))
                {
                    break;
                }
                count += retval;
            }
            printf("读了%d\n", count);
            fclose(f);
            close(skt_fd);

            printf("发送完成\n");

            break;

        case 4:
            //群发功能
            printf("请输入要群发的内容：");
            bzero(buf, sizeof(buf));
            scanf("%s", buf);
            msg_info.msg_flag = msg_flag;
            broadcast_msg_data(ginfo.skt_fd, &msg_info, sizeof(msg_info));

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
