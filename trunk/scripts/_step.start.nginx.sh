#!/bin/bash
src_dir='src'
if [[ ! -d $src_dir ]]; then echo "错误：必须在src同目录执行脚本"; exit 1; fi

cmd="sudo ./objs/nginx/sbin/nginx"
echo "启动NGINX（HLS服务）：$cmd"
pids=`ps aux|grep nginx|grep process|awk '{print $2}'`; for pid in $pids; do echo "结束现有进程：$pid"; sudo kill -s SIGKILL $pid; done
sudo ./objs/nginx/sbin/nginx
ret=$?; if [[ 0 -ne $ret ]]; then echo "错误：启动NGINX（HLS服务）失败"; exit $ret; fi

echo "启动NGINX（HLS服务）成功"
exit 0
