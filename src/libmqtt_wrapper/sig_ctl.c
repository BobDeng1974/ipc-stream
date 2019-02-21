// Last Update:2019-02-21 19:07:04
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
#include "dbg_internal.h"
#include "sig_ctl.h"
#include "mqtt.h"
#include "control.h"
#include "queue.h"

#define MAX_RETRY 5
#define SESSION_MAX 10

typedef struct {
    MqttContex *ctx[SESSION_MAX];
    int index;
} MqttMgr;

static MqttMgr gMqttMgr;

MqttContex *GetMqttContex( const void *instance)
{
    int i = 0;

    if ( !instance ) {
        return NULL;
    }

    for ( i=0; i<gMqttMgr.index; i++ ) {
        if ( gMqttMgr.ctx[i] && 
             gMqttMgr.ctx[i]->pInstance == instance ) {
            return gMqttMgr.ctx[i];
        }
    }
    
    return NULL;
}

static void OnMessage( const void* _pInstance, int _nAccountId, const char* _pTopic,
                const char* _pMessage, size_t nLength )
{
    MqttContex *ctx = GetMqttContex( _pInstance );

    if ( !ctx ) {
        return;
    }

    if ( ctx->pTopic ) {
        LOG_E("get message _pInstance %p topic %s message %s\n", _pInstance,  _pTopic, _pMessage );
    }
    if (  ctx->pInstance && 
          ctx->pInstance == _pInstance ) {
        if ( ctx->q ) {
            ctx->q->enqueue( ctx->q, _pMessage, nLength );
        } else {
            LOG_E("mMqttContex->q is NULL\n");
        }
    } else {
        LOG_E("check pointer error\n");
    }

}

static void OnEvent(const void* _pInstance, int _nAccountId, int _nId,  const char* _pReason )
{
    MqttContex *ctx = GetMqttContex( _pInstance );

    if ( !_pInstance ) {
        LOG_E("check param error\n");
        return;
    }

    if ( !ctx ) {
        return;
    }

    LOGI("_pInstance %p id %d reason %s \n", _pInstance,  _nId, _pReason );
    if ( _nId == MQTT_SUCCESS ) {
        if (  ctx->pInstance && 
              _pInstance == ctx->pInstance ) {
            if ( ctx->pTopic ) {
                LOGI("instance : %p start to subscribe %s \n", ctx->pInstance, ctx->pTopic);
                LinkMqttSubscribe( ctx->pInstance, ctx->pTopic );
            }
            ctx->connected = 1;
        } 
    }

}

MqttContex * MqttNewContex( char *_pClientId, int qos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort )
{
    MqttContex *pContex = NULL;
    struct MqttOptions options, *ops = &options;
    static int nIsFirst = 1;

    if ( !_pClientId || !_pHost ) {
        LOG_E("check param error, _pClientId = %s, _pHost = %s, _pTopic = %s\n",
              _pClientId, _pHost, _pTopic );
        return NULL;
    }


    if ( nIsFirst ) {
        LinkMqttLibInit();
        nIsFirst = 0;
    }

    pContex = (MqttContex*) malloc ( sizeof(MqttContex) );
    if ( !pContex ) {
        free( pContex );
        LOG_E("malloc error\n");
        return NULL;
    }

    memset( pContex, 0, sizeof(MqttContex) );
    if ( _pTopic ) {
        int len = strlen( _pTopic );
        pContex->pTopic = (char *) malloc ( len+1 );
        if ( !pContex->pTopic ) {
            return NULL;
        }
        memset( pContex->pTopic, 0, len+1 );
        strncpy( pContex->pTopic, _pTopic, len );
    }

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
        LOG_E("LinkMqttCreateInstance error\n");
        goto err;
    }
    LOG_E("new mqtt instance : client = %s, broker = %s, port = %d, topic = %s\n", _pClientId, _pHost, _nPort, _pTopic );

    pContex->q = NewQueue();
    if ( !pContex->q ) {
        LOG_E("new queue error\n");
        goto err;
    }

    gMqttMgr.ctx[gMqttMgr.index++] = pContex;
    LOG_E("pContex->pInstance = %p\n", pContex->pInstance );
    int retry = 0;
    for ( retry = 0; retry < MAX_RETRY; retry ++ ) {
        if ( !pContex->connected ) {
            sleep( 1 );
        }
    }

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
            LOG_E("check param error\n");
            return;
        }
        if ( pContex->pInstance ) {
            LinkMqttDestroy( pContex->pInstance );
        }
        free( pContex );
    }

}

int MqttSend( MqttContex *_pConext, char *_pTopic,  char *_pMessage )
{
    if ( !_pConext || !_pMessage || !_pTopic  ) {
        LOG_E("check param error, _pConext = %p, _pMessage = %p, _pTopic = %p\n", _pConext, _pTopic, _pMessage );
        return -1;
    }

    if ( !_pConext ) {
        LOG_E("check pContex error\n");
        return -1;
    }

    if ( _pConext->connected )
        LinkMqttPublish( _pConext->pInstance, _pTopic, strlen(_pMessage), _pMessage );

    return 0;
}

int MqttRecv( MqttContex *_pConext, char *_pMsg, int *_pLen )
{
    if ( !_pConext || !_pMsg || !_pLen ) {
        LOG_E("check param error\n");
        return -1;
    }

    if ( !_pConext->q ) {
        LOG_E("check q error\n"); return -1;
    }

    _pConext->q->dequeue( _pConext->q, _pMsg, _pLen );

    return 0;
}


