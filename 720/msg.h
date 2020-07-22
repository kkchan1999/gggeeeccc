#ifndef _MSG_H_
#define _MSG_H_

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>

#include <pthread.h>

#define MSGSIZE 64

#define PROJ_PATH "."
#define PROJ_ID 1

struct msgbuf {
    long mtype;
    char mtext[MSGSIZE];
};

#endif