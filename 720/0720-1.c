/* 利用消息队列编写一个程序，程序在运行的时候可以指定自己的类型值是多少，跟我们通信的对面类型值是多少，然后将这个程序通过两个终端运行，实现聊天
	例如：
		第一个终端运行：
			自己的类型是1，跟我们通信的类型是2
			./program   1    2
		第二个终端运行：
			自己的类型是2，跟我们通信的类型是1
			./program   2    1
		这个程序接收到数据便打印出来，也可以从键盘中监控键盘的数据发送到对面实现聊天，当对方发送一句"exit"的时候，我们退出整个程序（父线程需要回收子线程的返回值，返回值为“exit”）
	结合线程的方式来做 */

#include "msg.h"

void* send_msg(void* arg)
{
    key_t key = ftok(PROJ_PATH, PROJ_ID);
    int id = msgget(key, IPC_CREAT | 0666);
    long type = *(long*)arg;

    struct msgbuf message;
    bzero(&message, sizeof(message));

    message.mtype = type;

    char* send_text;
    send_text = calloc(1, 128);
    while (1) {
        scanf("%s", send_text);
        if (msgsnd(id, &message, strlen(message.mtext), 0) != 0) {
            perror("msgsne() error");
            exit(1);
        }
    }

    strncpy(message.mtext, "sad", MSGSIZE);
}

//不用写接收的线程，直接在主线程搞就行

int main(int argc, char* argv[])
{
    if (argc != 3) {
        printf("参数输入有误！\n");
        return 0;
    }

    long send_type = atoi(argv[1]);
    long recv_type = atoi(argv[0]);

    key_t key = ftok(PROJ_PATH, PROJ_ID); //这一步是弄个消息队列的key
    int msgid = msgget(key, IPC_CREAT | 0666); //获取消息队列ID

    struct msgbuf buf;
    bzero(&buf, sizeof(buf)); //弄一个buf

    while (1) {
        if (msgrcv(msgid, &buf, MSGSIZE, recv_type, 0) == -1) {
            perror("msgrcv()error");
            exit(1);
        }
        if (buf.mtype == recv_type) {
            if (buf.mtext == "exit") { //bug,这样写必定退不出去
                pthread_exit(NULL); //接收到exit就退出
            }

            printf("收到来自%ld的信息：%s\n", recv_type, buf.mtext);
        }
    }
}
