// Last Update:2018-12-27 14:17:36
/**
 * @file sig_ctl.c
 * @brief 
 * @author liyq
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
#include <signal.h>
#include "dbg.h"
#include "sig_ctl.h"
#include "mqtt.h"
#include "control.h"

static MqttContex *mMqttContex;

static void OnMessage( const void* _pInstance, int _nAccountId, const char* _pTopic,
                const char* _pMessage, size_t nLength )
{
    LOGI("get message topic %s message %s\n", _pTopic, _pMessage );

    if ( mMqttContex->pCb ) {
        mMqttContex->pCb( _pMessage, nLength );
    }
}

static void OnEvent(const void* _pInstance, int _nAccountId, int _nId,  const char* _pReason )
{
    LOGI(" id %d reason %s \n", _nId, _pReason );
    if ( !_pInstance ) {
        LOGE("check param error\n");
        return;
    }

    if ( _nId == 3000 && mMqttContex ) {
        LOGI("start to subscribe %s \n", mMqttContex->pTopic);
        LinkDinitIOCtrl( mMqttContex->nSession );
        mMqttContex->nSession = LinkInitIOCtrl( NULL, NULL, _pInstance );
    }

}

MqttContex * MqttNewContex( char *_pClientId, MqttQoS _nQos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort, MqttMessageCb _pCb )
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

    LOGI("pContex = %p\n", pContex );

    memset( pContex, 0, sizeof(MqttContex) );

    pContex->qos = _nQos;
    LOGI("_pTopic = %s\n", _pTopic );
    pContex->pTopic = _pTopic;
    pContex->pCb = _pCb;

    memset( ops, 0, sizeof(struct MqttOptions) );
    ops->pId = _pClientId;
    ops->bCleanSession = false;
    ops->userInfo.nAuthenicatinMode = MQTT_AUTHENTICATION_NULL;
    ops->userInfo.pHostname = _pHost;
    ops->userInfo.nPort = _nPort;
    ops->userInfo.pCafile = NULL;
    ops->userInfo.pCertfile = NULL;
    ops->userInfo.pKeyfile = NULL;
    ops->nKeepalive = 10;
    ops->nQos = 0;
    ops->bRetain = false;
    ops->callbacks.OnMessage = &OnMessage;
    ops->callbacks.OnEvent = &OnEvent;
    pContex->pInstance = LinkMqttCreateInstance( ops );
    if ( !pContex->pInstance ) {
        LOGE("LinkMqttCreateInstance error\n");
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

int MqttSend( MqttContex *_pConext, char *_pMessage, int _nLen )
{
    MqttContex * pContex = NULL;

    if ( !_pConext || !_pMessage || _nLen <= 0 ) {
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


