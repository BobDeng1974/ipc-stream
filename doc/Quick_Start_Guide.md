# 目录 | Quick Start Guide

[TOC]

# 1. 概述

设备端SDK是七牛开发的适用于安防监控场景的摄像头端SDK，提供了音视频实时点对点播放，实时消息通信，音视频云端存储，摄像头云端录像回放等功能。可以帮助用户快速、高效的开发出安防监控场景的摄像头产品。

## 1.1 功能模块及版本号
| 模块 | 描述 | 版本 |
|---|---|---|
| TSUPLOADER | 音视频切片上传到云端模块 | 0.0.1 |
| MQTT | 实时消息通信模块 | 0.0.1 |
| TUTK | 点对点播放模块 | 0.0.1 |

# 2. 开发准备

## 2.1 设备以及系统要求
- 设备要求：设备端为 同时支持 H.264 和 JPEG 硬编码的视频处理芯片
- 设备系统要求：Linux Kernel >= 3.4

## 2.2 开发环境准备
1. 准备一个 Linux 操作系统 ( Ubuntu 14.04 或更高版本 )；
2. 安装交叉编译工具链；
3. 将 SDK 库文件和头文件加入到工程目录；
4. 引用库函数并验证功能；


# 3. 快速开始

## 3.1 视频切片上传 SDK 开发流程
切片上传( TSUPLOADER )模块将原始H.264视频流和原始AAC音频流进行 MPEG2-TS 封装，并上传到 七牛对象存储服务器。

- 首先需要使用 `LinkInit()` 初始化切片上传SDK, 然后构建 `LinkUploadArg` 结构体，将上传参数写入该结构体，具体参数请参考SDK手册，将 `LinkUploadArg` 结构体作为参数传给 `LinkNewUploader()` 生成一个上传实例。 
- 摄像头端底层SDK通常会提供视频和音频的回调函数，将 `LinkPushVideo()` `LinkPushAudio()` 函数分别注册到回调函数中，底层生成的原始音视频数据将会传递给SDK进行切片封装和上传。<br>
- 最后使用 `LinkFreeUploader()` 释放切片上传实例，并通过 `LinkCleanup()` 释放切片上传模块资源。


## 3.2 MQTT 消息通道 SDK 开发流程
MQTT 模块基于 MQTT 协议为摄像头端提供一套实时消息通信机制。<br>

- 使用 `MqttInit()` 初始化模块； 通过`MqttCreateInstance()` 创建一个消息实例；在创建消息的同时要注册 `MqttCallback` 消息和事件通知结构体，详见SDK参考手册，接收到的订阅消息通过这个回调函数通知。
- 通过 `MqttPub()` 发布消息；通过 `MqttSub()` 和 ` MqttUnsub()` 订阅和取消订阅相关主题的消息。<br> 
- 最后使用 `MqttDestroyInstance()` 释放 MQTT 实例，并通过 `MqttCleanup()` 释放 MQTT 模块资源。


## 3.3 TUTK 相关 SDK 开发流程
TUTK 模块通过建立点对点通道实现视频的实时播放。该文档基于 TUTK SDK 3.1.5.41 版本进行说明。 <br>
设备端使用到 TUTK 的 IOTC 模块 和 AV 模块。 其中 IOTC 模块用于设备的登录和验证，以及基础通信的建立；AV 模块用于音视频数据的实时点对点传输。

- 首先使用 `IOTC_Initialize()` 和 `avInitialize()` 初始化 IOTC 模块和 AV 模块资源，然后通过 `IOTC_Device_Login()` 登录到 IOTC 服务平台.可以使用 `IOTC_Get_Login_Info()` 获取登录信息。 
- 登录成功后使用 `IOTC_Listen()` 建立会话通道， 并建立 AV 服务 `avServStart()`
- 将 `avSendFrameData()` 和 `avSendAudioData()` 注册到摄像头底层 音视频回调函数中。
- 使用 `avServStop()` 停止 AV 服务，并通过 `avDeInitialize()` 和 `IOTC_DeInitialize()` 注销TUTK模块资源。





