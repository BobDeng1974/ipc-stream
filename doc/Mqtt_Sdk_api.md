<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [目录 | MQTT SDK API](#%E7%9B%AE%E5%BD%95--mqtt-sdk-api)
- [1. 概述](#1-%E6%A6%82%E8%BF%B0)
- [2. 接口说明](#2-%E6%8E%A5%E5%8F%A3%E8%AF%B4%E6%98%8E)
  - [2.1 MQTT消息通道接口](#21-mqtt%E6%B6%88%E6%81%AF%E9%80%9A%E9%81%93%E6%8E%A5%E5%8F%A3)
      - [2.1.1 初始化 MQTT SDK](#211-%E5%88%9D%E5%A7%8B%E5%8C%96-mqtt-sdk)
      - [2.1.2 注销 MQTT SDK](#212-%E6%B3%A8%E9%94%80-mqtt-sdk)
      - [2.1.3 创建一个 MQTT 实例](#213-%E5%88%9B%E5%BB%BA%E4%B8%80%E4%B8%AA-mqtt-%E5%AE%9E%E4%BE%8B)
      - [2.1.4 销毁一个 MQTT 实例](#214-%E9%94%80%E6%AF%81%E4%B8%80%E4%B8%AA-mqtt-%E5%AE%9E%E4%BE%8B)
      - [2.1.5 上报 MQTT 消息](#215-%E4%B8%8A%E6%8A%A5-mqtt-%E6%B6%88%E6%81%AF)
      - [2.1.6 订阅 MQTT 消息](#216-%E8%AE%A2%E9%98%85-mqtt-%E6%B6%88%E6%81%AF)
      - [2.1.7 取消订阅 MQTT 消息](#217-%E5%8F%96%E6%B6%88%E8%AE%A2%E9%98%85-mqtt-%E6%B6%88%E6%81%AF)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# 目录 | MQTT SDK API

[TOC]

# 1. 概述

主要提供MQTT协议的SDK

# 2. 接口说明

## 2.1 MQTT消息通道接口

#### 2.1.1 初始化 MQTT SDK

```c
/**
 * @brief 初始化 MQTT SDK
 *
 * @return MQTT_ERR_CODE
 */
int LinkMqttLibInit();
```

#### 2.1.2 注销 MQTT SDK

```c
/**
 * @brief 注销 MQTT SDK
 *
 * @return MQTT_ERR_CODE
 */
int LinkMqttLibCleanup();
```

#### 2.1.3 创建一个 MQTT 实例

```c
/**
 * @brief 创建一个 MQTT 实例
 *
 * @param[in]  pMqttpOption 创建的 MQTT 参数
 * @return 创建成功的 MQTT 实例指针
 */
void * LinkMqttCreateInstance( const MqttOptions* pMqttpOption );
```

#### 2.1.4 销毁一个 MQTT 实例

```c
/**
 * @brief 销毁一个 MQTT 实例
 *
 * @param[in] pInstance 需要销毁的MQTT实例
 * @return MQTT_ERR_CODE
 */
int LinkMqttDestroyInstance(const void* pInstance);
```

#### 2.1.5 上报 MQTT 消息

```c
/**
 * @brief 上报 MQTT 消息
 *
 * @param[in] pInstance MQTT实 例
 * @param[in] pTopic 发布主题
 * @param[in] nPayloadlen 发布消息长度
 * @param[in] pPayload 发布消息负载
 * @return MQTT_ERR_CODE
 */
int LinkMqttPublish(const void* pInstance,
                    char* pTopic,
                    size_t nPayloadlen,
                    const void* pPayload
                    );
```

#### 2.1.6 订阅 MQTT 消息

```c
/**
 * @brief 订阅 MQTT 消息
 *
 * @param[in] pInstance MQTT 实例
 * @param[in] pTopic 订阅主题
 * @return
 */
int LinkMqttSubscribe(const void* pInstance,
                      char* pTopic
                      );
```

#### 2.1.7 取消订阅 MQTT 消息

```c
/**
 * 取消订阅 MQTT 消息
 *
 * @param[in] pInstance MQTT 实例
 * @param[in] pTopic 取消订阅主题
 * @return
 */
int LinkMqttUnsubscribe(const void* pInstance,
                        char* pTopic
                        );
```

