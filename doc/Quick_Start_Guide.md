# 目录 | Quick Start Guide

[TOC]

# 1. 概述

RTMP推流SDK是七牛推出的适用于ipc的SDK，提供了RTMP推流和MQTT信令的功能.可以帮助客户快速、高效的开发出安防监控场景的摄像头产品.

## 1.1 功能模块及版本号
| 模块 | 描述 | 版本 |
|---|---|---|
| libmqtt | MQTT信令模块 | 0.0.1 |
| librtmp | RTMP推流模块 | 0.0.1 |

# 2. 开发准备

## 2.1 设备以及系统要求
- 设备要求：设备端为 支持 H.264/H.265/aac/g711 硬编码的音视频处理芯片
- 设备系统要求：Linux Kernel >= 3.4

## 2.2 开发环境准备
- 准备一个 Linux 操作系统 ( Ubuntu 14.04 或更高版本 )；
- 安装交叉编译工具链；
- 将 SDK 库文件和头文件加入到工程目录；
- 安装camke : apt-get instal cmake
- 移植ipcam_sdk 
	- git submodule update --init --recursive
	- cd ipcam_sdk
	- make CROSS_PREFIX=XXX(交叉编译工具链前缀)
	- 编译之后会生成三个静态库：libfdk-aac.a librtmp.a librtmp_sdk.a
- 移植wolfmqtt
   - cd wolfMQTT
   - /configure --prefix=$PWD/output --host=arm-linux-gnueabihf CC=arm-linux-gnueabihf-gcc CPP=arm-linux-gnueabihf-cpp  --enable-tls=no --enable-static ( arm-linux-gnueabihf要替换成自己的交叉编译工具链 )
   - make && make install
   - 在output目录下会生成libwolfmqtt.a



# 3. 快速开始

## 3.1 关于sample
- hal目录是对ipc的抽象，将ipc的能力抽象出一个通用的函数操作集。用于支持多个ipc厂商的设备，一个新的ipc厂商如果想让自己的设备能够工作起来，只需要实现CaptureDevice结构体里面的成员，并注册到dev_core，不需要改动其他的代码，就能够使sample工作起来
	- CaptureDevice结构体说明
		- init 摄像头初始化，并注册音频和视频的回调函数
		- deInit 去初始化，可以在此函数释放资源
		- getDevId 获取设备id
		- startStream 开始音视频流
		- isAudioEnable 音频是否是能
		- registerAlarmCb 注册事件回调，比如移动侦测，抓图
		- captureJpeg 抓图函数
		- stopStream 停止音视频流
	- 某个具体的ipc填充好CaptureDevice这个结构体之后，调用函数CaptureDeviceRegister注册到dev_core
	- devices目录下是所有ipc设备存放的目录，sample中使用原始的h264和aac文件模拟出了一个ipc：ipc_simulator.c
- 整体的RTMP推流流程可以参照main.c

## 3.2  RTMP推流开发流程
首先进行必要的初始化，接下来等待信令，接收到推流开始的信令后，开始推流。接收到推流停止的信令，结束推流。


- 初始化 

    - 创建MQTT实例(LinkMqttCreateInstance)
    - 创建RTMP实例(RtmpNewContext)
    - 初始化摄像头，注册视音频回调函数
    
- 循环等待信令( LinkRecvIOCtrl )
- 接收到推流开始信令，回复response( LinkSendIOResponse )
- 启动摄像头的视音频流
- 在视频帧的回调里面，发送RTMP流( RtmpSendVideo )
- 在音频帧的回调里面，发送RTMP流（RtmpSendAudio ）


