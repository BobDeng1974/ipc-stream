# 目录 | MQTT SDK API

[TOC]

# 1. 概述

主要提供MQTT协议的SDK

# 2. 接口说明

## 2.1 初始化 MQTT SDK

```c
/**
 * @brief 初始化 MQTT SDK
 *
 * @return MQTT_ERR_CODE
 */
int LinkMqttLibInit();
```

## 2.2 注销 MQTT SDK

```c
/**
 * @brief 注销 MQTT SDK
 *
 * @return MQTT_ERR_CODE
 */
int LinkMqttLibCleanup();
```

## 2.3 创建一个 MQTT 实例

```c
/**
 * @brief 创建一个 MQTT 实例
 *
 * @param[in]  pMqttpOption 创建的 MQTT 参数
 * @return 创建成功的 MQTT 实例指针
 */
void * LinkMqttCreateInstance( const MqttOptions* pMqttpOption );
```

## 2.4 销毁一个 MQTT 实例

```c
/**
 * @brief 销毁一个 MQTT 实例
 *
 * @param[in] pInstance 需要销毁的MQTT实例
 * @return MQTT_ERR_CODE
 */
int LinkMqttDestroyInstance(const void* pInstance);
```

## 2.5 上报 MQTT 消息

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

## 2.6 订阅 MQTT 消息

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

## 2.7 取消订阅 MQTT 消息

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

