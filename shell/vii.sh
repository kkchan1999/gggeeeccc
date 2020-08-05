#!/bin/bash

file=$1 #创建这个文件

echo "#include <stdio.h>" > $file
echo "#include <stdlib.h>" >> $file

vi $file