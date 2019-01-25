// Last Update:2019-01-25 17:21:42
/**
 * @file sig_ctl.h
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2018-12-18
 */

#ifndef SIG_CTL_H
#define SIG_CTL_H

#include <pthread.h>
#include "queue.h"

typedef struct {
    void *pInstance;
    void *pubInstance;
    char *pTopic;
    int connected;
    Queue *q;
    Queue *pubQ;
    pthread_mutex_t pubMutex;
    pthread_cond_t pubCond;
} MqttContex;

extern MqttContex * MqttNewContex( char *_pClientId, int _nQos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort );
extern void MqttDestroyContex( MqttContex *_pConext );
extern int MqttSend( MqttContex *_pConext, char *_pMessage );
extern int MqttRecv( MqttContex *_pConext, char *_pMsg, int *_pLen );

#endif  /*SIG_CTL_H*/
