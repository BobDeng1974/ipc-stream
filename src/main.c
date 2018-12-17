// Last Update:2018-12-17 18:47:46
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
#include "dbg.h"
#include "dev_core.h"
#include "rtmp_wapper.h"

typedef struct {
    char *pUrl;
    int nTimeout;
    int nInputAudioType;
    int nOutputAudioType;
    int nTimePolic;
    RtmpPubContext *pContext;
    pthread_mutex_t mutex;
} app_t;

static app_t app = 
{
    .pUrl = "rtmp://pili-publish.caster.test.cloudvdn.com/caster-test/test18",
    .nTimeout = 10,
    .nInputAudioType = RTMP_PUB_AUDIO_AAC, 
    .nOutputAudioType = RTMP_PUB_AUDIO_AAC,
    .nTimePolic = RTMP_PUB_TIMESTAMP_ABSOLUTE,
};

int VideoFrameCallBack ( char *_pFrame, 
                   int _nLen, int _nIskey, double _dTimeStamp, 
                   unsigned long _nFrameIndex, unsigned long _nKeyFrameIndex, 
                   int streamno )
{
    int ret = 0;

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendVideo( app.pContext, _pFrame, _nLen, _nIskey, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        LOGE("RtmpSendVideo error\n");
    }
    pthread_mutex_unlock( &app.mutex );

    return 0;
}

int AudioFrameCallBack( char *_pFrame, int _nLen, double _dTimeStamp,
                     unsigned long _nFrameIndex, int streamno )
{
    int ret = 0;

    pthread_mutex_lock( &app.mutex );
    ret = RtmpSendAudio( app.pContext, _pFrame, _nLen, (unsigned int) _dTimeStamp );
    if ( ret < 0 ) {
        LOGE("RtmpSendAudio error\n");
    }
    pthread_mutex_unlock( &app.mutex );
    return 0;
}

int main()
{
    CoreDevice *pDev = NewCoreDevice();
    int ret = 0;

    if ( !pDev ) {
        LOGE("NewCoreDevice() error\n");
        return 0;
    }

    pthread_mutex_init( &app.mutex, NULL );
    app.pContext = RtmpNewContext( app.pUrl, app.nTimeout,
                                   app.nInputAudioType, app.nOutputAudioType, app.nTimePolic );
    if ( !app.pContext ) {
        LOGE("RtmpNewContext() error\n");
        return 0;
    }

    ret = RtmpConnect( app.pContext );
    if ( ret < 0 ) {
        LOGE("RtmpConnect error\n");
        return 0;
    }

    pDev->init( AUDIO_AAC, 0, VideoFrameCallBack, AudioFrameCallBack );
    pDev->startStream( STREAM_MAIN );

    for (;;) {
        LOGI("heart beat...\n");
        sleep( 6 );
    }

    return 0;
}

