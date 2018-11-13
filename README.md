# flyingkv cpp implementation

[TOC]

## 简介
一个插件式的单机存储引擎。

工程主要目录：

目录|说明
:---|:---
docs|文档
bin|各种可执行脚本工具(build、generate)
conf|配置文件
proto|rpc IDLs
codegen|pb生成代码
sys|封装的os底层接口
server|服务端入口
kv|kv状态机
rpc|封装的rpc框架
client|客户端
examples|例子

## 先决条件

项|值
:---|:---
OS|linux kernel >=3.10.0-327.36.4.el7.x86_64
gcc|>= 4.8.2

## build
脚本的使用参考各脚本的-h
### prepare
1. ./bin/download-thirdparty.sh
2. ./bin/install-thirdparty.sh

### flyingkv
./bin/build.sh -gen

### ut
./bin/build.sh -gen -ut all

### examples
./bin/build.sh -gen -eg

## run
### flyingkv
./flyingkvd --flagfile=./conf/flyingkv.conf

### ut
./bin/run-all-uts.sh

### examples
./flyingkveg

## TODO
1. add async rpc server, so we can support more concurrency requests and add a scheduler use queue to scheduler and control requests.
2. fix ut
3. use yaml?
4. fix TODO
5. fix fork waitpid unknown child process error(目前这个错误对正确性没有影响).
