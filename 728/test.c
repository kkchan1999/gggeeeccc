#include "apue.h"

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

#define BUFLEN 128

#define MAXSLEEP 128
int connect_retry(int domain, int type, int protocol, const struct sockaddr* addr, socklen_t alen)
{
    int numsec, fd;

    //try to connect with expontial backoff
    for (numsec = 1; numsec <= MAXSLEEP; numsec <<= 1) {
        if ((fd = socket(domain, type, protocol)) < 0) {
            return -1;
        }
        if (connect(fd, addr, alen) == 0) {
            //连接成功
            return fd;
        }
        close(fd);

        //延时之后再尝试
        if (numsec <= MAXSLEEP / 2) {
            sleep(numsec);
        }
    }
    return -1;
}

void print_uptime(int sockfd)
{
    int n;
    char buf[BUFLEN];

    while ((n = recv(sockfd, buf, BUFLEN, 0)) > 0) {
        write(STDOUT_FILENO, buf, n);
        if (n < 0) {
            err_sys("recv error");
        }
    }
}

int main(int argc, char const* argv[])
{
    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    int sockfd, err;

    if (argc != 2) {
        err_quit("usage: ruptime hostname");
    }

    memset(&hint, 0, sizeof(hint)); //这是什么函数？

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_canonname = NULL;
    hint.ai_addr = NULL;
    hint.ai_next = NULL;

    if ((err = getaddrinfo(argv[1], "ruptime", &hint, &ailist)) != 0) {
        err_quit("getaddrinfo error: %s", gai_strerror(err));
    }

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        }

    return 0;
}
