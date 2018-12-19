// Last Update:2018-12-19 11:28:24
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
#include "dbg.h"
#include "sig_ctl.h"

static MqttMessageCb pMessageCb;

void MessageArrived( MessageData* _pMessage )
{
    MQTTMessage* pMessage = _pMessage->message;

    if ( pMessage ) {
        LOGI("%d.%s\n", _pMessage->topicName->lenstring.len, _pMessage->topicName->lenstring.data );
        if ( pMessageCb ) {
            pMessageCb( (char*)pMessage->payload, (int)pMessage->payloadlen );
        } else {
            LOGE("pMessageCb is NULL\n");
        }
    } else {
        LOGE("get empty message\n");
    }
}

MqttContex * MqttNewContex( char *_pClientId, enum QoS _nQos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort, MqttMessageCb _pCb )
{
    int rc = 0;
    MqttContex *pContex = NULL;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    if ( !_pClientId ||  !_pHost || !_pCb ) {
        LOGE("check param error\n");
        return NULL;
    }

    pContex = (MqttContex *) malloc ( sizeof(MqttContex) );
    if ( !pContex ) {
        LOGE("malloc error\n");
        return NULL;
    }

    pContex->pSendBuf = ( unsigned char *) malloc ( 128 );
    if ( !pContex->pSendBuf ) {
        LOGE("malloc error\n");
        free( pContex );
        return NULL;
    }

    memset( pContex->pSendBuf, 0, 128 );
    pContex->pReadBuf = ( unsigned char *) malloc ( 128 );
    if ( !pContex->pReadBuf ) {
        LOGE("malloc error\n");
        free( pContex );
        return NULL;
    }
    memset( pContex->pReadBuf, 0, 128 );
    pContex->pTopic = _pTopic;

    pMessageCb = _pCb;
    NetworkInit( &pContex->n );
    NetworkConnect( &pContex->n, _pHost, _nPort );
    MQTTClientInit( &pContex->c, &pContex->n, 2000, pContex->pSendBuf, 128, pContex->pReadBuf, 128 );

    data.willFlag = 0;
    data.MQTTVersion = 3;
    data.clientID.cstring = _pClientId;
    data.username.cstring = _pUserName;
    data.password.cstring = _pPasswd;
    data.keepAliveInterval = 10;
    data.cleansession = 1;
    rc = MQTTConnect( &pContex->c, &data );
    LOGI("rc = %d\n", rc );
    LOGI("Subscribing to %s\n", _pTopic );
    rc = MQTTSubscribe( &pContex->c, _pTopic, _nQos, MessageArrived );
    LOGI("rc = %d\n", rc );

    return pContex;
}

void MqttDestroyContex( MqttContex *_pConext )
{
    if ( _pConext ) {
        MQTTDisconnect( &_pConext->c );
        NetworkDisconnect( &_pConext->n );
        free( _pConext->pSendBuf );
        free( _pConext->pReadBuf );
        free( _pConext );
    }
}

int MqttYield( MqttContex *_pConext, int nTimeOut )
{
    if ( _pConext ) {
        return ( MQTTYield( &(_pConext->c), nTimeOut) );
    }

    return -1;
}

int MqttSend( MqttContex *_pConext, char *_pMessage, int _nLen )
{
    MQTTMessage message;
    int rc = 0;

    if ( !_pConext || !_pMessage || _nLen <= 0 ) {
        LOGE("check param error\n");
        return -1;
    }

    message.qos = 1;
    message.retained = 0;
    message.payload = _pMessage;;
    message.payloadlen = _nLen;
    rc = MQTTPublish( &(_pConext->c), _pConext->pTopic, &message ); 
    if ( rc != 0 ) {
        LOGE("MQTTPublish() error, rc = %d\n", rc );
        return -1;
    }

    return 0;
}


