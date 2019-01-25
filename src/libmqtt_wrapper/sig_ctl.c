// Last Update:2019-01-25 15:04:09
/**
 * @file sig_ctl.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2018-12-17
 */
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "dbg.h"
#include "sig_ctl.h"
#include "mqtt.h"
#include "control.h"
#include "queue.h"

static MqttContex *mMqttContex;

static void OnMessage( const void* _pInstance, int _nAccountId, const char* _pTopic,
                const char* _pMessage, size_t nLength )
{
    LOGI("get message topic %s message %s\n", _pTopic, _pMessage );
    if ( mMqttContex 
         && mMqttContex->pInstance 
         && mMqttContex->pInstance == _pInstance ) {
        if ( mMqttContex->q ) {
            mMqttContex->q->enqueue( mMqttContex->q, _pMessage, nLength );
        } else {
            LOGE("mMqttContex->q is NULL\n");
        }
    } else {
        LOGE("check pointer error\n");
    }

}

static void OnEvent(const void* _pInstance, int _nAccountId, int _nId,  const char* _pReason )
{
    if ( !_pInstance ) {
        LOGE("check param error\n");
        return;
    }

    LOGI(" id %d reason %s \n", _nId, _pReason );
    if ( _nId == MQTT_SUCCESS && mMqttContex ) {
        if ( mMqttContex && mMqttContex->pInstance && mMqttContex->pTopic
             && _pInstance == mMqttContex->pInstance )
            LOGI("instance : %p start to subscribe %s \n", _pInstance, mMqttContex->pTopic);
            LinkMqttSubscribe( mMqttContex->pInstance, mMqttContex->pTopic );
    }

}

MqttContex * MqttNewContex( char *_pClientId, int qos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort )
{
    MqttContex *pContex = NULL;
    struct MqttOptions options, *ops = &options;

    if ( !_pClientId || !_pTopic || !_pHost ) {
        LOGE("check param error\n");
        return NULL;
    }


    LinkMqttLibInit();

    pContex = (MqttContex*) malloc ( sizeof(MqttContex) );
    if ( !pContex ) {
        free( pContex );
        LOGE("malloc error\n");
        return NULL;
    }

    pthread_mutex_init( &pContex->mutex, NULL );
    pthread_cond_init( &pContex->cond, NULL );

    memset( pContex, 0, sizeof(MqttContex) );
    pContex->pTopic = _pTopic;

    memset( ops, 0, sizeof(struct MqttOptions) );
    ops->pId = _pClientId;
    ops->bCleanSession = false;
    ops->userInfo.nAuthenicatinMode = MQTT_AUTHENTICATION_NULL;
    ops->userInfo.pHostname = _pHost;
    ops->userInfo.nPort = _nPort;
    ops->userInfo.pCafile = NULL;
    ops->userInfo.pCertfile = NULL;
    ops->userInfo.pKeyfile = NULL;
    ops->nKeepalive = 15;
    ops->nQos = 0;
    ops->bRetain = false;
    ops->callbacks.OnMessage = &OnMessage;
    ops->callbacks.OnEvent = &OnEvent;
    pContex->pInstance = LinkMqttCreateInstance( ops );
    if ( !pContex->pInstance ) {
        LOGE("LinkMqttCreateInstance error\n");
        goto err;
    }

    pContex->q = NewQueue();
    if ( !pContex->q ) {
        LOGE("new queue error\n");
        goto err;
    }

    mMqttContex = pContex;
    return pContex;

err:
    free( pContex );
    return NULL;
}


void MqttDestroyContex( MqttContex *_pConext )
{
    MqttContex *pContex = NULL;

    if ( _pConext ) {
        pContex = _pConext;
        if ( !pContex ) {
            LOGE("check param error\n");
            return;
        }
        if ( pContex->pInstance ) {
            LinkMqttDestroy( pContex->pInstance );
        }
        free( pContex );
    }

}

int MqttSend( MqttContex *_pConext, char *_pMessage )
{
    MqttContex * pContex = NULL;

    if ( !_pConext || !_pMessage  ) {
        LOGE("check param error\n");
        return -1;
    }

    pContex = _pConext;
    if ( !pContex ) {
        LOGE("check pContex error\n");
        return -1;
    }

    LinkMqttPublish( pContex->pInstance, pContex->pTopic, 10, _pMessage );

    return 0;
}

int MqttRecv( MqttContex *_pConext, char *_pMsg, int *_pLen )
{
    if ( !_pConext || !_pMsg || !_pLen ) {
        LOGE("check param error\n");
        return -1;
    }

    if ( !_pConext->q ) {
        LOGE("check q error\n");
        return -1;
    }

    _pConext->q->dequeue( _pConext->q, _pMsg, _pLen );

    return 0;
}


