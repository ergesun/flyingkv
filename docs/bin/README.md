# 编译使用介绍

## 说明
  bin文件夹下的脚本供下载依赖、编译、清理、运行使用

## os环境
  linux(当前不支持windows)

## 脚本介绍(bin/*)
  - download-thirdparty.sh              : 下载第三方依赖。
  - install-thirdparty.sh               : 编译、安装第三方依赖的脚本。
  - build.sh                            : 默认release版本（可发布版本），如果传入 **-d** 参数，会编译成debug版本供调试;如果输入 **-dl** ，会输出debug log。
                                          **-gen**会调用generate-protobuf-idls.sh生成codegen。**-ut**编译单元测试。**-eg**编译例子!!注意: **-d** 和 **-dl** 是两个事。
  - clean.sh                            : 清理所有编译生成的除codegen的产物。加 **-gen** 参数支持清理codegen。
  - rebuild.sh                          : 先清理，后编译。支持 **-d -dl -gen** 参数。
  - generate-protobuf-idls.sh           : 生成pb。
  - run-all-uts.sh                      : 执行所有已编译的单元测试(执行前需要先编译)。如果编译时加了**-d**参数，那么执行本脚本也需要加。

## 配置介绍(conf/*)
  - acc.json : 访问控制配置。
  - flyingkv.conf : 服务的主要配置文件。

## 编译方法
  1. 必要工具(请自行安装)：
     - cmake   笔者构建版本3.2
     - gcc/g++ 笔者构建版本4.9.2,由于gcc ABI并不是所有版本都向下兼容(其实gcc版本不低于4.8.2即可)，
       比如说5.1版本libstdc++.so有的内容就不向前兼容了，所以如果你想 升级/降级 gcc/g++，需要注意
  2. 执行download-thirdparty.sh(初次需要)
  3. 执行install-thridparty.sh(初次需要)
  4. 执行build.sh -gen或rebuild.sh -gen(-gen参数初次需要或者有idls改动的时候需要)

## 测试执行方法
  1. 根据编译方法编译出可执行文件
  2. 配置conf下的flyingkv.conf配置文件
  3. 执行./flyingkvd --flagfile=./flyingkv.conf

