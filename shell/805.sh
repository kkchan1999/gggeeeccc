#!/bin/bash

my_func()
{
    wc -w $1
    return 0
}


sighand()
{
    echo "接收到中断信号"

    echo "请输入文件名"
    read fileName 

    my_func $fileName

    return 0
}


trap sighand INT #注册信号

while true
do
    #死循环
    echo ""
    sleep 10
done