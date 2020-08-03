#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>

typedef struct client_info {
    int client_fd;
    char name[256];
    char ip[16];
    short port;
    pthread_t tid;

    struct client_info* next;
} client_info_node;

#define list_for_each(head, pos) \
    for (pos = head->next; pos != NULL; pos = pos->next)

//申请客户端信息链表的头节点
static client_info_node* request_client_info_node(const client_info_node* info)
{
    client_info_node* new_node;

    new_node = malloc(sizeof(client_info_node));
    if (new_node == NULL) {
        perror("申请客户端节点异常");
        return NULL;
    }

    if (info != NULL)
        *new_node = *info;

    new_node->next = NULL;

    return new_node;
}

static inline void insert_client_info_node_to_link_list(client_info_node* head, client_info_node* insert_node)
{
    client_info_node* pos;

    for (pos = head; pos->next != NULL; pos = pos->next)
        ;

    pos->next = insert_node;
}

void* client_thread(void* arg)
{
    int client_fd;
    char buffer[1024];
    ssize_t recv_size, send_size;
    client_info_node *pos, *list_head = arg;

    list_for_each(list_head, pos)
    {
        if (pthread_self() == pos->tid) {
            client_fd = pos->client_fd;
            break;
        }
    }

    while (1) {
        bzero(buffer, sizeof(buffer));

        /*
			接受来自与客户端的数据，这个接受具备阻塞特性
			跟read函数差不多，都是指定从client_fd里面读取sizeof(buffer)长的数据放到buffer当中，0则代表按照默认操作读取数据
		*/
        recv_size = recv(client_fd, buffer, sizeof(buffer), 0);
        if (recv_size == -1) {
            perror("接受数据异常");
            break;
        } else if (recv_size == 0) //代表客户端断开连接
        {
            //删除链表节点
            break;
        }

        list_for_each(list_head, pos)
        {
            if (pos->client_fd == client_fd)
                continue;

            printf("转发数据给端口号为%hu\n", pos->port);

            send_size = send(pos->client_fd, buffer, recv_size, 0);
            if (send_size == -1)
                break;
        }
        printf("接收到来自与客户端%ld个字节的数据：%s\n", recv_size, buffer);
    }

    return NULL;
}

int main(void)
{
    int skt_fd;
    int retval;
    client_info_node *list_head, *new_node;
    client_info_node cache_client_info;

    list_head = request_client_info_node(NULL);

    /*
		获取程序通信的套接字（接口）
		AF_INET：IPV4的协议
		SOCK_STREAM：指定TCP协议
		0：代表不变化协议内部（ip手册中指定的参数）
	*/
    skt_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (skt_fd == -1) {
        perror("申请套接字失败");
        return -1;
    }

    struct sockaddr_in native_addr;

    native_addr.sin_family = AF_INET; //指定引用IPV4的协议

    native_addr.sin_port = htons(6666); //指定端口号，转化为网络字节序（大端序）

    //inet_aton(INADDR_ANY, &(native_addr.sin_addr));

    native_addr.sin_addr.s_addr = htonl(INADDR_ANY); //将所有的IP地址转化为二进制的网络字节序的数据进行绑定

    /*
		将指定的地址信息及本程序的套接字绑定在一起
		skt_fd：套接字的文件描述符
		&native_addr：需要绑定的地址信息结构体（每个协议的地址信息结构体是不一样的，我们用IPV4的协议便需要引用struct sockaddr_in）
		sizeof(native_addr)：传入的结构体长度
	*/
    retval = bind(skt_fd, (struct sockaddr*)&native_addr, sizeof(native_addr));
    if (retval == -1) {
        perror("绑定套接字地址失败");
        goto bind_socket_err;
    }

    /*
		设置套接字的同时通信最大连接数为50，并且将这个套接字的属性设置为可监听属性
	*/
    retval = listen(skt_fd, 50);
    if (retval == -1) {
        perror("设置最大连接数失败");
        goto set_listen_err;
    }

    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t sklen = sizeof(client_addr);

    while (1) {

        /*
			等待客户端链接，链接成功后返回一个代表客户端通信的文件描述符,具备阻塞特性
			skt_fd：代表套接字接口
			client_addr：链接成功后客户端的地址信息会存放到这里面
			sklen：代表结构体的长度
		*/
        cache_client_info.client_fd = accept(skt_fd, (struct sockaddr*)&client_addr, &sklen);
        if (cache_client_info.client_fd == -1) {
            perror("客户端链接失败");
            goto client_connect_err;
        }

        strcpy(cache_client_info.ip, inet_ntoa(client_addr.sin_addr)); //存放IP地址
        cache_client_info.port = ntohs(client_addr.sin_port);

        //新建节点
        new_node = request_client_info_node(&cache_client_info);
        //将节点插入链表
        insert_client_info_node_to_link_list(list_head, new_node);

        printf("服务器：客户端连接成功\n");
        printf("客户端信息：\n客户端IP为%s，端口号为%hu\n", cache_client_info.ip, cache_client_info.port);

        pthread_create(&(new_node->tid), NULL, client_thread, list_head);
    }

    close(client_fd); //关闭客户端通信
    close(skt_fd); //关闭服务器的socket资源

    return 0;

socket_recv_err:
    close(client_fd);
client_connect_err:
set_listen_err:
bind_socket_err:
    close(skt_fd);
    return -1;
}
