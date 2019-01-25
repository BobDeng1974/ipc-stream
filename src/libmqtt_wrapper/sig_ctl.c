// Last Update:2019-01-25 18:11:30
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

#define MAX_RETRY 5
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
        if (  mMqttContex->pInstance && mMqttContex->pTopic
             && _pInstance == mMqttContex->pInstance ) {
            LOGI("instance : %p start to subscribe %s \n", _pInstance, mMqttContex->pTopic);
            LinkMqttSubscribe( mMqttContex->pInstance, mMqttContex->pTopic );
            mMqttContex->connected = 1;
        } 
    }

}

void *MqttMonitoringTask( void *arg )
{
    MqttContex *pContex = (MqttContex *)arg;

    if ( !pContex ) {
        LOGE("check param error\n");
        return NULL;
    }

    for (;;) {
        pthread_mutex_lock( &pContex->pubMutex );
        LOGI("wait for cond\n");
        pthread_cond_wait( &pContex->pubCond, &pContex->pubMutex );

        if ( pContex->pubQ && pContex->connected ) {
            char message[128] = { 0 };
            int len = 0;

            pContex->pubQ->dequeue( pContex->pubQ, message, &len );
            if ( pContex->pInstance && len ) {
                LOGI("publish topic : %s msg : %s \n", pContex->pTopic, message );
                LinkMqttPublish( pContex->pInstance, pContex->pTopic, strlen(message), message );
            } else {
                LOGE("check instance or len error\n");
            }
        } else {
            LOGE("pubQ is NULL\n");
        }

        pthread_mutex_unlock( &pContex->pubMutex );
    }

    return NULL;
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

    pthread_mutex_init( &pContex->pubMutex, NULL );
    pthread_cond_init( &pContex->pubCond, NULL );

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

    char clientId[64] = { 0 };

    pContex->q = NewQueue();
    if ( !pContex->q ) {
        LOGE("new queue error\n");
        goto err;
    }

    pContex->pubQ = NewQueue();
    if ( !pContex->pubQ ) {
        LOGE("new queue error\n");
        goto err;
    }

    mMqttContex = pContex;

    int retry = 0;
    for ( retry = 0; retry < MAX_RETRY; retry ++ ) {
        if ( !pContex->connected ) {
            sleep( 1 );
        }
    }

    pthread_t thread;
    pthread_create( &thread, NULL, MqttMonitoringTask, pContex );

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

    if ( pContex->pubQ ) {
        pContex->pubQ->enqueue( pContex->pubQ, _pMessage, strlen(_pMessage) );
        pthread_mutex_lock( &pContex->pubMutex );
        pthread_cond_signal( &pContex->pubCond );
        pthread_mutex_unlock( &pContex->pubMutex );
    }

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


