// Last Update:2018-12-27 11:05:54
/**
 * @file main.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2018-12-11
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "log.h"
#include "dbg.h"
#include "dev_core.h"
#include "rtmp_wapper.h"
#include "sig_ctl.h"
#include "mqtt.h"
#include "control.h"

typedef struct {
    char *pSignal;
    int nSignal;
} MqttSignal;

typedef struct {
    char *pUrl;
    int nTimeout;
    int nInputAudioType;
    int nOutputAudioType;
    int nTimePolic;
    char *pClientId;
    MqttQoS nQos;
    char *pUserName;
    char *pPasswd;
    char *pTopic;
    char *pHost;
    int nPort;
    int nStreamSts;
    MqttContex *pMqttContex;
    RtmpPubContext *pContext;
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
    .pUrl = "rtmp://pili-publish.caster.test.cloudvdn.com/caster-test/test18",
    .nTimeout = 10,
    .nInputAudioType = RTMP_PUB_AUDIO_AAC, 
    .nOutputAudioType = RTMP_PUB_AUDIO_AAC,
    .nTimePolic = RTMP_PUB_TIMESTAMP_ABSOLUTE,
    .pClientId = "ipc-rtmp-mqtt-208",
    .nQos = 2,
    .pUserName = NULL,
    .pPasswd = NULL,
    .pTopic = "pushLive",
    .pHost = "emqx.qnservice.com",
    .nPort = 1883,
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

/* 6.视频帧回调，摄像头采集到一帧h264图像，调用此回调，调用接口RtmpSendVideo发送视频流 */
int VideoFrameCallBack ( char *_pFrame, 
                   int _nLen, int _nIskey, double _dTimeStamp, 
                   unsigned long _nFrameIndex, unsigned long _nKeyFrameIndex, 
                   int streamno )
{
    int ret = 0;

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendVideo( app.pContext, _pFrame, _nLen, _nIskey, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        app.pDev->stopStream();
        LOGE("RtmpSendVideo error\n");
    }
    pthread_mutex_unlock( &app.mutex );

    return 0;
}

/* 7.音频帧回调，摄像头采集到帧aac音频数据，调用此回调，调用接口RtmpSendAudio发送音频流 */
int AudioFrameCallBack( char *_pFrame, int _nLen, double _dTimeStamp,
                     unsigned long _nFrameIndex, int streamno )
{
    int ret = 0;

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendAudio( app.pContext, _pFrame, _nLen, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        LOGE("RtmpSendAudio error\n");
        app.pDev->stopStream();
    }
    pthread_mutex_unlock( &app.mutex );
    return 0;
}

void EventLoop()
{
    char message[1000] = { 0 };
    unsigned int nIOCtrlType = 0;
    int nSize = 1000, nSignal = 0;
    int ret = 0;
    char *resp = "pushSucceed";

    ret = LinkRecvIOCtrl( app.pMqttContex->nSession, &nIOCtrlType, message, &nSize, 6000 );
    if ( ret == MQTT_SUCCESS ) {
        LOGI("message = %s\n", message );
        nSignal = GetMqttSignal( message );
        switch( nSignal ) {
        case pushLiveStart:
            if ( app.pDev ) {

                LOGI("get signal pushLiveStart, start to push rtmp stream\n");
                app.pDev->startStream( STREAM_MAIN );
                ret = LinkSendIOResponse( app.pMqttContex->nSession, 0, resp, strlen(resp) );
                LOGI("ret = %d\n", ret );
                LOGI("set app stream running\n");
            }
            break;
        case pushLiveStop:
            if ( app.pDev ) {
                LOGI("get signal pushLiveStop, stop to push rtmp stream\n");
                app.pDev->stopStream();
            }
            break;
        case pushSucceed:
            LOGI("pushSucceed\n");
            break;
        default:
            break;
        }

    } else if ( ret != MQTT_RETRY  && ret != MQTT_ERR_INVAL ){
        LOGI("ret = %d\n", ret );
    }
}

void SdkLogCallBack( char *log )
{
    LOGI( log );
}

int main()
{
    int ret = 0;

    app.nStreamSts = STREAM_STATUS_STOPED;

    SetLogCallBack( SdkLogCallBack );
    LoggerInit( 1, OUTPUT_FILE, "/tmp/ipc-rtmp-stream.log", 1 );

    /* 1.初始化mqtt信令 */
    LOGI("init mqtt\n");
    app.pMqttContex = MqttNewContex( app.pClientId, app.nQos, app.pUserName, app.pPasswd,
                                     app.pTopic, app.pHost, app.nPort, NULL ) ;
    if ( !app.pMqttContex ) {
        LOGE("MqttNewContex error\n");
        return 0;
    }

    /* 2.初始化rtmp推流 */
    LOGI("init rtmp lib\n");
    pthread_mutex_init( &app.mutex, NULL );
    app.pContext = RtmpNewContext( app.pUrl, app.nTimeout,
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

    for (;;) {
        char memUsed[16] = { 0 };
        static int count = 0;

        /* 5.循环接收app信令，收到pushLiveStart，开始rmtp推流 */
        EventLoop();

        DbgGetMemUsed( memUsed );
        if ( count == 16 ) {
            count = 0;
            LOGI("memory used : %s\n", memUsed );
        }
        count++;
    }

    return 0;
}

