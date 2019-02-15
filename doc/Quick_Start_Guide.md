# 目录 | Quick Start Guide

[TOC]

# 1. 概述

本文档主要指导用户基于七牛mqtt和rtmp推流库快速实现可控的RTMP推流。

# 2 功能模块及版本号
| 模块 | 描述 | 版本 |
|---|---|---|
| libmqtt | MQTT信令模块 | 0.0.1 |
| librtmp | RTMP推流模块 | 0.0.1 |


# 3. 快速开始

## 3.1 开发流程
 
- 创建MQTT实例，传入mqtt服务器地址，端口，主题等（ **LinkMqttCreateInstance** ）
- 创建RTMP实例，传入RTMP推流地址，端口，音视频编码参数等（ **RtmpPubNew** ）
- 初始化摄像头，注册视音频回调函数
- 在视频帧的回调里面，发送RTMP流( **RtmpPubSendVideoKeyframe/RtmpPubSendVideoKeyframe** )
- 在音频帧的回调里面，发送RTMP流（ **RtmpPubSendAudioFrame** ）
- 等待信令
- 收到“开始”信令，启动推流
- 收到“暂停”信令，停止推流

## 3.2 流程图
![Aaron Swartz](FlowChat.png)

# 4 sample介绍
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




