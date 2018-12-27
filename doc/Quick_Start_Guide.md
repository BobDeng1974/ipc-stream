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
1. 准备一个 Linux 操作系统 ( Ubuntu 14.04 或更高版本 )；
2. 安装交叉编译工具链；
3. 将 SDK 库文件和头文件加入到工程目录；
4. 引用库函数并验证功能；


# 3. 快速开始

## 3.1  RTMP推流开发流程
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


