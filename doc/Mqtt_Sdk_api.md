<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [目录 | Device SDK Manual](#%E7%9B%AE%E5%BD%95--device-sdk-manual)
- [1. 概述](#1-%E6%A6%82%E8%BF%B0)
- [2. 接口说明](#2-%E6%8E%A5%E5%8F%A3%E8%AF%B4%E6%98%8E)
  - [2.1 MQTT消息通道接口](#21-mqtt%E6%B6%88%E6%81%AF%E9%80%9A%E9%81%93%E6%8E%A5%E5%8F%A3)
    - [2.1.1 MQTT 返回错误码](#211-mqtt-%E8%BF%94%E5%9B%9E%E9%94%99%E8%AF%AF%E7%A0%81)
    - [2.2.2 MQTT 主要数据结构](#222-mqtt-%E4%B8%BB%E8%A6%81%E6%95%B0%E6%8D%AE%E7%BB%93%E6%9E%84)
    - [2.2.3 MQTT 接口说明](#223-mqtt-%E6%8E%A5%E5%8F%A3%E8%AF%B4%E6%98%8E)
      - [2.2.3.1 初始化 MQTT SDK](#2231-%E5%88%9D%E5%A7%8B%E5%8C%96-mqtt-sdk)
      - [2.2.3.2 注销 MQTT SDK](#2232-%E6%B3%A8%E9%94%80-mqtt-sdk)
      - [2.2.3.3 创建一个 MQTT 实例](#2233-%E5%88%9B%E5%BB%BA%E4%B8%80%E4%B8%AA-mqtt-%E5%AE%9E%E4%BE%8B)
      - [2.2.3.4 销毁一个 MQTT 实例](#2234-%E9%94%80%E6%AF%81%E4%B8%80%E4%B8%AA-mqtt-%E5%AE%9E%E4%BE%8B)
      - [2.2.3.5 上报 MQTT 消息](#2235-%E4%B8%8A%E6%8A%A5-mqtt-%E6%B6%88%E6%81%AF)
      - [2.2.3.6 订阅 MQTT 消息](#2236-%E8%AE%A2%E9%98%85-mqtt-%E6%B6%88%E6%81%AF)
      - [2.2.3.7 取消订阅 MQTT 消息](#2237-%E5%8F%96%E6%B6%88%E8%AE%A2%E9%98%85-mqtt-%E6%B6%88%E6%81%AF)
      - [2.2.3.8 接收信令](#2238-%E6%8E%A5%E6%94%B6%E4%BF%A1%E4%BB%A4)
      - [2.2.3.9 发送信令](#2239-%E5%8F%91%E9%80%81%E4%BF%A1%E4%BB%A4)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# 目录 | Device SDK Manual

[TOC]

# 1. 概述

主要提供MQTT协议的SDK

# 2. 接口说明

## 2.1 MQTT消息通道接口

### 2.1.1 MQTT 返回错误码

```c
 /** @brief MQTT 返回错误码 */
typedef enum _MQTT_ERR_CODE {
    MQTT_SUCCESS =              -3000,  /**< 表示成功调用API接口 */
    MQTT_CONNECT_SUCCESS =      -3001,  /**< 表示连接成功，回调接口返回 */
    MQTT_DISCONNECT_SUCCESS =   -3002,  /**< 表示断开连接成功，回调接口返回 */
    MQTT_ERR_NOMEM =            -3003,  /**< 没有内存 */
    MQTT_ERR_PROTOCOL =         -3004,  /**< 协议错误 */
    MQTT_ERR_INVAL =            -3005,  /**< 无效参数返回的错误 */
    MQTT_ERR_NO_CONN =          -3006,  /**< 没有链接响应 */
    MQTT_ERR_CONN_REFUSED =     -3007,  /**< 链接被拒绝 */
    MQTT_ERR_NOT_FOUND =        -3008,  /**< 没有发现服务器 */
    MQTT_ERR_CONN_LOST =        -3009,  /**< 链接丢失 */
    MQTT_ERR_TLS =              -3010,  /**< TLS 错误 */
    MQTT_ERR_PAYLOAD_SIZE =     -3011,  /**< 数据包大小不正确 */
    MQTT_ERR_NOT_SUPPORTED =    -3012,  /**< 不支持该调用 */
    MQTT_ERR_AUTH =             -3013,  /**< 认证不正确 */
    MQTT_ERR_ACL_DENIED =       -3014,  /**< 没有 ACL 权限 */
    MQTT_ERR_UNKNOWN =          -3015,  /**< 未知错误 */
    MQTT_ERR_PROXY =            -3016,  /**< 代理不正确 */
    MQTT_ERR_OTHERS =           -3017,  /**< 其他错误 */
    MQTT_SUCCESS = 0                    /**< 操作成功 */
} MQTT_ERR_CODE;
```

### 2.2.2 MQTT 主要数据结构

MQTT 授权模式

```c
/** @brief 授权模式 */
typedef enum _MQTT_AUTH_TYPE{
    MQTT_AUTH_NULL = 0x0,       /**< 无用户名和密码 */
    MQTT_AUTH_USER = 0x1,       /**< 需要设置用户名和密码 */
    MQTT_AUTH_ONEWAY_SSL = 0x2, /**< 需要单向认证 */
    MQTT_AUTH_TWOWAY_SSL = 0x4  /**< 需要双向认证 */
} MQTT_AUTH_TYPE;
```

MQTT 服务等级

```c
/** @brief MQTT QOS 等级 */
typedef enum _MQTT_LEVEL {
    MQTT_LEVEL_0 = 0,   /**< MQTT QOS LEVEL 0 */
    MQTT_LEVEL_1 = 1,   /**< MQTT QOS LEVEL 1 */
    MQTT_LEVEL_2 = 2    /**< MQTT QOS LEVEL 2 */
} MQTT_LEVEL;
```

MQTT 用户信息结构体

```c
/** @brief MQTT 用户信息结构体 */
typedef struct _MqttUserInfo {
    MQTT_AUTH_TYPE nAuthenicatinMode; /**< 授权模式 */
    char *pUsername;                /**< 用户名 */
    char *pPassword;                /**< 密码 */
    char *pHostname;                /**< MQTT服务器地址 */
    uint16_t nPort;                 /**< MQTT服务器端口 */
    char *pCafile;                  /**< 服务器CA证书文件路径 */
    char *pCertfile;                /**< 客户端授权证书文件路径，双向认证服务器端需要 */
    char *pKeyfile;                 /**< 客户端授权密钥，双向认证服务器端需要 */
    char *reserved1;                /**< 预留 */
} MqttUserInfo;
```

MQTT 参数结构体

```c
/** @brief MQTT 参数结构体 */
typedef struct _MqttOptions {
    char *pid;                       /**< 客户端id */
    bool bCleanSession;              /**< 是否清除会话 */
    MqttUserInfo primaryUserInfo;    /**< 首用户信息 */
    int32_t nKeepalive;              /**< 心跳 */
    MqttCallback callbacks;          /**< 用户回调函数 */
    MQTT_LEVEL nQos;                 /**< 服务质量 */
    bool bRetain;                    /**< 是否保留 */
} MqttOptions;
```

消息回调函数结构体

```c
/** @brief 消息回调函数结构体 */
typedef struct _MqttCallback {
    /** 消息回调函数, 用来接收订阅消息 */
    void (*OnMessage)(const void* _pInstance, const char* _pTopic, const char* _pMessage, const size_t _nLength);
    /** 事件回调函数, 用来接收关键消息和错误消息 */
    void (*OnEvent)(const void* _pInstance, const int _nCode, const char* _pReason);
} MqttCallback;

```

### 2.2.3 MQTT 接口说明

#### 2.2.3.1 初始化 MQTT SDK

```c
/**
 * @brief 初始化 MQTT SDK
 *
 * @return MQTT_ERR_CODE
 */
int MqttInit();
```

#### 2.2.3.2 注销 MQTT SDK

```c
/**
 * @brief 注销 MQTT SDK
 *
 * @return MQTT_ERR_CODE
 */
int MqttCleanup();
```

#### 2.2.3.3 创建一个 MQTT 实例

```c
/**
 * @brief 创建一个 MQTT 实例
 *
 * @param[out] pInstance 创建成功的 MQTT 实例指针
 * @param[in]  pMqttpOption 创建的 MQTT 参数
 * @return MQTT_ERR_CODE
 */
int MqttCreateInstance(void ** pInstance,
                       const MqttOptions* pMqttpOption
                       );
```

#### 2.2.3.4 销毁一个 MQTT 实例

```c
/**
 * @brief 销毁一个 MQTT 实例
 *
 * @param[in] pInstance 需要销毁的MQTT实例
 * @return MQTT_ERR_CODE
 */
int MqttDestroyInstance(const void* pInstance);
```

#### 2.2.3.5 上报 MQTT 消息

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
int MqttPub(const void* pInstance,
            char* pTopic,
            size_t nPayloadlen,
            const void* pPayload
            );
```

#### 2.2.3.6 订阅 MQTT 消息

```c
/**
 * @brief 订阅 MQTT 消息
 *
 * @param[in] pInstance MQTT 实例
 * @param[in] pTopic 订阅主题
 * @return
 */
int MqttSub(const void* pInstance,
            char* pTopic
            );
```

#### 2.2.3.7 取消订阅 MQTT 消息

```c
/**
 * 取消订阅 MQTT 消息
 *
 * @param[in] pInstance MQTT 实例
 * @param[in] pTopic 取消订阅主题
 * @return
 */
int MqttUnsub(const void* pInstance,
              char* pTopic
              );
```
#### 2.2.3.8 接收信令
```c
/**
 * 接收信令
 *
 * @param[in] nSession 会话id
 * @param[out] _pIOCtrlType 信令类型
 * @param[out] _pIOCtrlData 信令值
 * @param[in] _nIOCtrlMaxDataSize 输出信令buffer大小
 * @param[in] _nTimeout 超时时间
 * @return
 */
int LinkRecvIOCtrl(int nSession,
                  unsigned int *_pIOCtrlType,
                  char *_pIOCtrlData,
                  int *_nIOCtrlMaxDataSize,
                  unsigned int _nTimeout
                  );
```

#### 2.2.3.9 发送信令
```c
/**
 * 发送信令
 *
 * @param[in] nSession 会话id
 * @param[in] _nIOErrorCode 错误码
 * @param[in] _pIOCtrlData 信令值
 * @param[in] _nIOCtrlDataSize 输入的信令buffer大小
 * @return
 */

int LinkSendIOResponse(int nSession,
                       unsigned int _nIOErrorCode,
                       const char *_pIOCtrlData,
                       int _nIOCtrlDataSize
                       );

```
