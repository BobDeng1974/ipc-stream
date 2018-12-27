# 目录 | Device SDK Manual

[TOC]

# 1. 概述

设备端SDK主要提供两个功能：

1. RTMP推流
2. mqtt信令

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

### 2.2.4 RTMP推流接口

#### 2.2.4.1 创建推流实例
```c
/**
 *  创建推流实例
 *
 * @param[in] _url 推流的url
 * @param[in] _nTimeout 发送及接受超时时间,单位：秒
 * @param[in] _nInputAudioType 音频输入类型
 * @param[in] _nOutputAudioType 音频输出类型
 * @param[in] _nTimePolic RTMP_PUB_TIMESTAMP_ABSOLUTE数据包使用绝对时间戳发送；
 *             RTNP_PUB_TIMESTAMP_RELATIVE 数据包尽量使用相对时间戳发送，如果时间发生回转，
 *             会发送绝对时间戳进行时间戳同步
 * @return rtmp实例
 */
RtmpContex * RtmpNewContext( const char * _url,
                              unsigned int _nTimeout,
                              RtmpPubAudioType _nInputAudioType,
                              RtmpPubAudioType _nOutputAudioType,
                              RtmpPubTimeStampPolicy _nTimePolic
                              );

```

#### 2.2.4.2 发送视频帧数据
```c
/**
 * 发送视频帧数据 
 *
 * @param[in] _pConext 推流实例句柄
 * @param[in] _pData 图像数据
 * @param[in] _nSize 图像数据大小
 * @param[in] _nIsKey 是否为关键帧
 * @param[in] _nPresentationTime 时间戳
 * @return -1 : 失败 0 : 成功
 */
int RtmpSendVideo( RtmpContex *_pConext,
                   char *_pData,
                   unsigned int _nSize,
                   int _nIsKey,
                   unsigned int _nPresentationTime 
                   );

```

#### 2.2.4.3 发送音频帧数据
```c
/**
 * 发送音频帧数据 
 *
 * @param[in] _pConext 推流实例句柄
 * @param[in] _pData 音频数据
 * @param[in] _nSize 音频数据大小
 * @param[in] _nPresentationTime 时间戳
 * @return -1 : 失败 0 : 成功
 */

int RtmpSendAudio( RtmpContex *_pConext,
                   char *_pData,
                   unsigned int _nSize,
                   unsigned int _nPresentationTime )


```
#### 2.2.4.4 连接推流服务器
```c
/**
 * 连接推流服务器
 *
 * @param[in] _pConext 推流实例句柄
 * @return -1 : 失败 0 : 成功
 */

int RtmpConnect( RtmpContex * _pConext)

```

#### 2.2.4.5 销毁推流实例句柄
```c
/**
 * 销毁推流实例句柄
 *
 * @param[in] _pConext 推流实例句柄
 * @return -1 : 失败 0 : 成功
 */
int RtmpDestroy( RtmpContex * _pConext );
```
