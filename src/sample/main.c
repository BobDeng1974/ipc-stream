// Last Update:2019-02-21 19:18:07
/**
 * @file main.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2018-12-11
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "dbg.h"
#include "dev_core.h"
#include "rtmp_wapper.h"
#include "sig_ctl.h"

typedef struct {
    char *pSignal;
    int nSignal;
} MqttSignal;

typedef struct {
    char url[512];
    int nTimeout;
    int nInputAudioType;
    int nOutputAudioType;
    int nTimePolic;
    char *pClientId;
    int nQos;
    char *pUserName;
    char *pPasswd;
    char *pTopic;
    char *pHost;
    char *pCfgPath;
    char *cache;
    int nCacheLen;
    int nPort;
    int nStreamSts;
    char *pLogTopic;
    char *pClientIdLog;
    MqttContex *pMqttContex;
    RtmpContex *pContext;
    pthread_mutex_t mutex;
    CoreDevice *pDev;
} app_t;

#define ITEM_LIST \
    ADD_SIGNAL_ITEM( pushLiveStart ) \
    ADD_SIGNAL_ITEM( pushLiveStop ) \
    ADD_SIGNAL_ITEM( pushSucceed )

#define ADD_SIGNAL_ITEM( item ) item,

enum {
   ITEM_LIST 
};

#undef ADD_SIGNAL_ITEM
#define ADD_SIGNAL_ITEM( item ) { #item, item },
#define ARRSZ(arr) (sizeof(arr)/sizeof(arr[0]))

static MqttSignal gSignalList[] = 
{
    ITEM_LIST
};

enum {
    STREAM_STATUS_RUNNING,
    STREAM_STATUS_STOPED,
};

static app_t app = 
{
    .nTimeout = 10,
    .nInputAudioType = RTMP_PUB_AUDIO_AAC, 
    .nOutputAudioType = RTMP_PUB_AUDIO_AAC,
    .nTimePolic = RTMP_PUB_TIMESTAMP_ABSOLUTE,
    .pClientId = "ipc-rtmp-mqtt-208-9",
    .nQos = 2,
    .pUserName = NULL,
    .pPasswd = NULL,
    .pTopic = "pushLive",
    .pHost = "emqx.qnservice.com",
    .nPort = 1883,
    .pCfgPath = "/tmp/oem/app/ipc-rtmp.conf",
    .pLogTopic = "rtmp-stream-208",
    .pClientIdLog = "rtmp-stream-208",
    
};

int GetMqttSignal( char *pMqttSignal )
{
    int i = 0;

    for ( i=0; i<ARRSZ(gSignalList); i++ ) {
        if ( strncmp( gSignalList[i].pSignal, pMqttSignal,
                      strlen(gSignalList[i].pSignal) ) == 0 ) {
            return gSignalList[i].nSignal;
        }
    }

    return -1;
}

int RtmpReconnect()
{
    int ret = 0;

    RtmpDestroy( app.pContext );
    app.pContext = RtmpNewContext( app.url, app.nTimeout,
                                   app.nInputAudioType, app.nOutputAudioType, app.nTimePolic );
    if ( !app.pContext ) {
        LOGE("RtmpNewContext() error");
        return -1;
    }

    ret = RtmpConnect( app.pContext );
    if ( ret < 0 ) {
        LOGE("RtmpConnect error");
        return -1;
    }

    LOGI("reconnect OK");
    return 0;
}

/* 6.视频帧回调，摄像头采集到一帧h264图像，调用此回调，调用接口RtmpSendVideo发送视频流 */
int VideoFrameCallBack ( char *_pFrame, 
                   int _nLen, int _nIskey, double _dTimeStamp, 
                   unsigned long _nFrameIndex, unsigned long _nKeyFrameIndex, 
                   int streamno )
{
    int ret = 0;
    static int i = 0, nIsFirst = 1;

    if ( i == 5000 ) {
        LOGI("%s called", __FUNCTION__ );
        i = 0;
    }
    i++;

    if ( app.nStreamSts != STREAM_STATUS_RUNNING ) {
        return 0;
    }

    if ( i > 5 && !_nIskey && nIsFirst ) {
        app.cache = (char *)malloc( _nLen );
        if ( !app.cache ) {
            LOGE("malloc error");
        }
        memcpy( app.cache, _pFrame, _nLen );
        app.nCacheLen = _nLen;
        nIsFirst = 0;
    }

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendVideo( app.pContext, _pFrame, _nLen, _nIskey, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        static int j = 0;

        RtmpReconnect();
        if ( j == 50 ) {
            LOGE("RtmpSendVideo error");
            j = 0;
        } 
        j++;
    }
    pthread_mutex_unlock( &app.mutex );

    return 0;
}

/* 7.音频帧回调，摄像头采集到帧aac音频数据，调用此回调，调用接口RtmpSendAudio发送音频流 */
int AudioFrameCallBack( char *_pFrame, int _nLen, double _dTimeStamp,
                     unsigned long _nFrameIndex, int streamno )
{
    int ret = 0;
    static int i = 0;


    if ( i == 5000 ) {
        LOGI("%s called", __FUNCTION__ );
        i = 0;
    }
    i++;

    if ( app.nStreamSts != STREAM_STATUS_RUNNING ) {
        return 0;
    }

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendAudio( app.pContext, _pFrame, _nLen, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        static int i = 0;

        if ( i == 100 ) {
            LOGE("RtmpSendAudio error, errno = %d", errno );
            i = 0;
        }
        i++;
        RtmpReconnect();
    }
    pthread_mutex_unlock( &app.mutex );
    return 0;
}

void EventLoop()
{
    char message[1000] = { 0 };
    int nSize = 1000, nSignal = 0;
    int ret = 0;
    char *resp = "pushSucceed";

    // 8. 等待mqtt信令
    ret = MqttRecv( app.pMqttContex, message, &nSize );
    if ( ret == 0 ) {
        LOGI("message = %s", message );
        nSignal = GetMqttSignal( message );
        switch( nSignal ) {
        case pushLiveStart:
            if ( app.pDev ) {

                LOGI("get signal pushLiveStart, start to push rtmp stream");
                app.nStreamSts = STREAM_STATUS_RUNNING;
                // 9. 发送response
                ret = MqttSend( app.pMqttContex, app.pTopic, resp );
                LOGI("ret = %d", ret );
            }
            break;
        case pushLiveStop:
            if ( app.pDev ) {
                LOGI("get signal pushLiveStop, stop to push rtmp stream");
                app.nStreamSts = STREAM_STATUS_STOPED;
            }
            break;
        case pushSucceed:
            LOGI("pushSucceed");
            break;
        default:
            break;
        }

    } 
}

int LoadPushUrl()
{
    FILE *fp = fopen( app.pCfgPath, "r" );
    int size = 0;

    if ( !fp ) {
        LOGE("open file %s error", app.pCfgPath );
        return -1;
    }

    fseek( fp, 0L, SEEK_END );
    size = ftell(fp);
    if ( size == 0 ) {
        LOGE("file %s no url", app.pCfgPath );
        return -1;
    }
    rewind( fp );
    fread( app.url, 1, size-1, fp );// delete \n
    LOGE("url = %s", app.url );
    fclose( fp );

    return 0;
}

/*
 * the mqtt server will close socket connection if no data coming in 30s
 * so we need to send heart beat packet to server
 * */
void *HeartBeatTask( void *arg )
{
    int ret = 0;

    for (;;) {
        if ( app.cache && app.nStreamSts == STREAM_STATUS_STOPED ) {
            ret = RtmpSendVideo( app.pContext, app.cache, app.nCacheLen, 0, 123456 );
            if ( ret < 0 ) {
                LOGE("send heart beatt error, ret = %d, reconnect rtmp stream", ret );
                RtmpReconnect();
            } else {
                LOGI("send heart beat packet success");
            }
            sleep( 20 );
        } else {
            sleep( 2 );
        }
    }
    return NULL;
}

int GetMemUsed( char *memUsed )
{
    char line[256] = { 0 }, key[32] = { 0 }, value[32] = { 0 };
    FILE *fp = NULL;
    char *ret = NULL;

    fp = fopen( "/proc/self/status", "r" );
    if ( !fp ) {
        printf("open /proc/self/status error" );
        return -1;
    }

    for (;;) {
        memset( line, 0, sizeof(line) );
        ret = fgets( line, sizeof(line), fp );
        if (ret) {
            sscanf( line, "%s %s", key, value );
            if (strcmp( key, "VmRSS:" ) == 0 ) {
                memcpy( memUsed, value, strlen(value) );
                fclose( fp );
                return 0;
            }
        }
    }

    fclose( fp );
    return -1;
}

int main( int argc , char *argv[] )
{
    int ret = 0;
    LogParam param;
    MqttParam mqtt;

    app.nStreamSts = STREAM_STATUS_STOPED;

    memset( &mqtt, 0, sizeof(MqttParam) );
    memset( &param, 0, sizeof(LogParam) );
    mqtt.clientId = app.pClientIdLog;
    mqtt.qos = app.nQos;
    mqtt.user = app.pUserName;
    mqtt.passwd = app.pPasswd;
    mqtt.topic = app.pLogTopic;
    mqtt.broker = app.pHost;
    mqtt.port = app.nPort;
    param.output = OUTPUT_MQTT;
    param.showTime = 1;
    param.level = DBG_LEVEL_INFO;
    param.verbose = 1;
    param.mqttParam = &mqtt;
    LoggerInit( &param );
    LoadPushUrl();

    /* 1.初始化mqtt信令 */
    LOGI("init mqtt");
    app.pMqttContex = MqttNewContex( app.pClientId, app.nQos, app.pUserName, app.pPasswd,
                                     app.pTopic, app.pHost, app.nPort ) ;
    if ( !app.pMqttContex ) {
        LOGE("MqttNewContex error");
        return 0;
    }

    /* 2.初始化rtmp推流 */
    LOGI("init rtmp lib");
    pthread_mutex_init( &app.mutex, NULL );
    app.pContext = RtmpNewContext( app.url, app.nTimeout,
                                   app.nInputAudioType, app.nOutputAudioType, app.nTimePolic );
    if ( !app.pContext ) {
        LOGE("RtmpNewContext() error");
        return 0;
    }

    /* 3.连接rtmp推流服务器 */
    ret = RtmpConnect( app.pContext );
    if ( ret < 0 ) {
        LOGE("RtmpConnect error");
        return 0;
    }

    /* 4.初始化网络摄像头，注册视音频帧回调 */
    LOGI("start to init ipc");
    app.pDev = NewCoreDevice();
    if ( !app.pDev ) {
        LOGE("NewCoreDevice() error");
        return 0;
    }
    app.pDev->init( AUDIO_AAC, 0, VideoFrameCallBack, AudioFrameCallBack );
    app.pDev->startStream( STREAM_MAIN );

    pthread_t thread;
    pthread_create( &thread, NULL, HeartBeatTask, NULL );

    for (;;) {
        char memUsed[16] = { 0 };
        static int count = 0;

        /* 5.循环接收app信令，收到pushLiveStart，开始rmtp推流 */
        EventLoop();

        GetMemUsed( memUsed );
        if ( count == 30 ) {
            count = 0;
            LOGI("memory used : %s", memUsed );
        }
        count++;
    }

    return 0;
}

