#!/bin/bash
src_dir='src'
if [[ ! -d $src_dir ]]; then echo "错误：必须在src同目录执行脚本"; exit 1; fi

# step 1: build srs 
echo "编译SRS"
./configure --x86-x64 --with-nginx && make
ret=$?; if [[ 0 -ne $ret ]]; then echo "错误：编译SRS失败"; exit $ret; fi

echo "编译SRS成功"
