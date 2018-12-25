// Last Update:2018-12-25 16:11:54
/**
 * @file sig_ctl.h
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-18
 */

#ifndef SIG_CTL_H
#define SIG_CTL_H

#include "wolfmqtt/mqtt_client.h"

typedef void ( *MqttMessageCb )( char *message, int len );

typedef struct {
    void *pInstance;
    char *pTopic;
    MqttQoS qos;
    MqttMessageCb pCb;
    int nSession;
} MqttContex;

extern MqttContex * MqttNewContex( char *_pClientId, MqttQoS _nQos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort, MqttMessageCb _pCb );
extern void MqttDestroyContex( MqttContex *_pConext );
extern int MqttYield( MqttContex *_pConext, int nTimeOut );
extern int MqttSend( MqttContex *_pConext, char *_pMessage, int _nLen );

#endif  /*SIG_CTL_H*/
