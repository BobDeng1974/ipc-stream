// Last Update:2019-01-26 17:18:37
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
    .pClientId = "ipc-rtmp-mqtt-208-2",
    .nQos = 2,
    .pUserName = NULL,
    .pPasswd = NULL,
    .pTopic = "pushLive",
    .pHost = "emqx.qnservice.com",
    .nPort = 1883,
    .pCfgPath = "/tmp/oem/app/ipc-rtmp.conf"
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
        LOGE("RtmpNewContext() error\n");
        return -1;
    }

    ret = RtmpConnect( app.pContext );
    if ( ret < 0 ) {
        LOGE("RtmpConnect error\n");
        return -1;
    }

    LOGI("reconnect OK\n");
    return 0;
}

/* 6.视频帧回调，摄像头采集到一帧h264图像，调用此回调，调用接口RtmpSendVideo发送视频流 */
int VideoFrameCallBack ( char *_pFrame, 
                   int _nLen, int _nIskey, double _dTimeStamp, 
                   unsigned long _nFrameIndex, unsigned long _nKeyFrameIndex, 
                   int streamno )
{
    int ret = 0;
    static int i = 0;

    if ( i == 5000 ) {
        LOGI("%s called\n", __FUNCTION__ );
        i = 0;
    }
    i++;

    if ( app.nStreamSts != STREAM_STATUS_RUNNING ) {
        return 0;
    }

    if ( i > 5 && !_nIskey ) {
        app.cache = (char *)malloc( _nLen );
        if ( !app.cache ) {
            LOGE("malloc error\n");
        }
        memcpy( app.cache, _pFrame, _nLen );
        app.nCacheLen = _nLen;
    }

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendVideo( app.pContext, _pFrame, _nLen, _nIskey, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        static int j = 0;

        RtmpReconnect();
        if ( j == 50 ) {
            LOGE("RtmpSendVideo error\n");
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
        LOGI("%s called\n", __FUNCTION__ );
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
            LOGE("RtmpSendAudio error, errno = %d\n", errno );
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
        LOGI("message = %s\n", message );
        nSignal = GetMqttSignal( message );
        switch( nSignal ) {
        case pushLiveStart:
            if ( app.pDev ) {

                LOGI("get signal pushLiveStart, start to push rtmp stream\n");
                app.nStreamSts = STREAM_STATUS_RUNNING;
                //app.pDev->startStream( STREAM_MAIN );
                // 9. 发送response
                ret = MqttSend( app.pMqttContex, resp );
                LOGI("ret = %d\n", ret );
            }
            break;
        case pushLiveStop:
            if ( app.pDev ) {
                LOGI("get signal pushLiveStop, stop to push rtmp stream\n");
                app.nStreamSts = STREAM_STATUS_STOPED;
                /* if network disconnect, call app.pDev->stopStream() will block */
                //app.pDev->stopStream();
            }
            break;
        case pushSucceed:
            LOGI("pushSucceed\n");
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
        LOGE("open file %s error\n", app.pCfgPath );
        return -1;
    }

    fseek( fp, 0L, SEEK_END );
    size = ftell(fp);
    if ( size == 0 ) {
        LOGE("file %s no url\n", app.pCfgPath );
        return -1;
    }
    rewind( fp );
    fread( app.url, 1, size-1, fp );// delete \n
    LOGE("url = %s\n", app.url );
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
                LOGE("send heart beatt error\n");
            }
            sleep( 20 );
        } else {
            sleep( 2 );
        }
    }
    return NULL;
}

int main( int argc , char *argv[] )
{
    int ret = 0;

    app.nStreamSts = STREAM_STATUS_STOPED;

    LoggerInit( 1, OUTPUT_FILE, "/tmp/ipc-rtmp-stream.log", 1 );
    LoadPushUrl();

    /* 1.初始化mqtt信令 */
    LOGI("init mqtt\n");
    app.pMqttContex = MqttNewContex( app.pClientId, app.nQos, app.pUserName, app.pPasswd,
                                     app.pTopic, app.pHost, app.nPort ) ;
    if ( !app.pMqttContex ) {
        LOGE("MqttNewContex error\n");
        return 0;
    }

    /* 2.初始化rtmp推流 */
    LOGI("init rtmp lib\n");
    pthread_mutex_init( &app.mutex, NULL );
    app.pContext = RtmpNewContext( app.url, app.nTimeout,
                                   app.nInputAudioType, app.nOutputAudioType, app.nTimePolic );
    if ( !app.pContext ) {
        LOGE("RtmpNewContext() error\n");
        return 0;
    }

    /* 3.连接rtmp推流服务器 */
    ret = RtmpConnect( app.pContext );
    if ( ret < 0 ) {
        LOGE("RtmpConnect error\n");
        return 0;
    }

    /* 4.初始化网络摄像头，注册视音频帧回调 */
    LOGI("start to init ipc\n");
    app.pDev = NewCoreDevice();
    if ( !app.pDev ) {
        LOGE("NewCoreDevice() error\n");
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

        DbgGetMemUsed( memUsed );
        if ( count == 300 ) {
            count = 0;
            LOGI("memory used : %s\n", memUsed );
        }
        count++;
    }

    return 0;
}

