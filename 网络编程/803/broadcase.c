#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h> /* See NOTES */
#include <unistd.h>
/* According to earlier standards */
#include <net/if.h>
#include <net/if_arp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

int get_netcard_broadcase_addr(int skt_fd, char* netcard_name, char* ip_addr, int ip_len)
{
    int retval;
    struct ifreq ifr;
    struct sockaddr_in get_addr, cache_addr;
    unsigned int addr_numb;

    bzero(&ifr, sizeof(ifr)); //内存清空

    strcpy(ifr.ifr_name, netcard_name); //将网卡名字放入到ifr.ifr_name的内存当中，指定好网卡的名字

    retval = ioctl(skt_fd, SIOCGIFADDR, &ifr); //直接获取指定网卡的地址信息到ifr结构体里面
    if (retval != 0) {
        perror("获取指定网卡IP地址失败");
        return -1;
    }

    memcpy(&get_addr, &(ifr.ifr_addr), sizeof(get_addr)); //将地址信息拷贝到get_addr结构体里面拿来分析

    addr_numb = ntohl(get_addr.sin_addr.s_addr); //将网络字节数的二进制IP地址转化为本地字节序的二进制IP地址，方便我们下面做IP地址类型的判断

    printf("本机ip地址：%s\n", inet_ntoa(get_addr.sin_addr)); //将获取到的网卡IP地址打印出来

    if ((addr_numb & 0xe0000000) <= 0x60000000) //保留32位IP地址的前三位数据，并且判断，A类地址由于是0开头，所以保留前面3位的最大值是011和面都是0，十六进制数就是0x60000000
    {
        cache_addr.sin_addr.s_addr = htonl(addr_numb | 0x00ffffff); //如果他是A类地址，他的网络地址则是前8位二进制，剩下的24位都是主机地址，全部置1便是广播地址（255就是全部都是1），并且转化为网络字节序存放进去变量当中

        printf("这个是A类地址，广播地址为%s\n", inet_ntoa(cache_addr.sin_addr)); //将广播地址打印出来
    } else if ((addr_numb & 0xe0000000) <= 0xa0000000) //同理，B类地址10开头，保留3位则是101是最大值，所以十六进制是0xa0000000
    {
        cache_addr.sin_addr.s_addr = htonl(addr_numb | 0x0000ffff);

        printf("这个是B类地址，广播地址为%s\n", inet_ntoa(cache_addr.sin_addr));
    } else if ((addr_numb & 0xe0000000) <= 0xc0000000) //同理，C类地址110开头，保留3位则是110是最大值，所以十六进制是0xc0000000
    {
        cache_addr.sin_addr.s_addr = htonl(addr_numb | 0x000000ff);

        printf("这个是C类地址，广播地址为%s\n", inet_ntoa(cache_addr.sin_addr));
    } else
        printf("这个是D类地址（组播地址）");

    strncpy(ip_addr, inet_ntoa(cache_addr.sin_addr), ip_len);

    return 0;
}

//./udp
int main(int argc, const char* argv[])
{
    char buffer[1024] = "我来啦";
    int udp_fd;
    int retval;
    ssize_t send_size;
    char broadcase_addr[16];
    struct sockaddr_in native_addr, dest_addr, recv_addr;
    socklen_t skt_len = sizeof(struct sockaddr_in);

    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd == -1) {
        perror("申请套接字失败");
        goto request_socket_err;
    }

    int sw = 1;

    retval = setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &sw, sizeof(sw));
    if (retval == -1) {
        perror("设置程序允许广播出错");
        goto setsock_broadcase_err;
    }

    native_addr.sin_family = AF_INET;
    native_addr.sin_port = htons(6665);
    native_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    retval = bind(udp_fd, (struct sockaddr*)&native_addr, sizeof(native_addr));
    if (retval == -1) {
        perror("绑定异常");
        return -1;
    }

    //根据网卡名字获取IP地址是多少
    get_netcard_broadcase_addr(udp_fd, "ens38", broadcase_addr, sizeof(broadcase_addr));

    printf("获取的本地广播地址信息：broadcase_ip=%s\n", broadcase_addr);

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(6666);
    dest_addr.sin_addr.s_addr = inet_addr(broadcase_addr);

    send_size = sendto(udp_fd, buffer, strlen(buffer),
        0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (send_size == -1) {
        perror("发送UDP数据失败\n");
        return -1;
    }

    ssize_t recv_size;

    while (1) {
        bzero(buffer, sizeof(buffer));

        recv_size = recvfrom(udp_fd, buffer, sizeof(buffer),
            0, (struct sockaddr*)&recv_addr, &skt_len);
        if (recv_size == -1) {
            perror("接受UDP数据失败\n");
            break;
        }

        printf("读取到%ld字节的数据：%s\n", recv_size, buffer);
    }

    close(udp_fd);

    return 0;

setsock_broadcase_err:
bind_socket_err:
    close(udp_fd);
request_socket_err:
    return -1;
}
