// Last Update:2019-02-21 17:20:45
/**
 * @file log2mqtt.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-02-20
 */

#include <stdio.h>
#include <string.h>
#include "dbg.h"
#include "sig_ctl.h"

static MqttContex *ctx;
static char gTopic[32];

static int MqttOutput( char *log )
{
    if ( !log ) {
        return -1;
    }
    if ( ctx )
        MqttSend( ctx, gTopic, log );
    else {
        PRINT("MqttSend fail\n");
    }

    return 0;
}

static int MqttInit( void *arg )
{
    LogParam *param = (LogParam *) arg;

    if ( !param || !param->mqttParam ) {
        return -1;
    }

    strncpy( gTopic, param->mqttParam->topic, strlen(param->mqttParam->topic) );
    PRINT("\nclientId : %s\ntopic : %s\nbroker : %s\nport : %d\n",
          param->mqttParam->clientId,
          param->mqttParam->topic,
          param->mqttParam->broker,
          param->mqttParam->port );

    ctx = MqttNewContex( param->mqttParam->clientId, 
                         param->mqttParam->qos, 
                         param->mqttParam->user,
                         param->mqttParam->passwd,
                         NULL,
                         param->mqttParam->broker,
                         param->mqttParam->port ) ;
    return 0;
}

static int MqttDeInit()
{
    MqttDestroyContex( ctx );

    return 0;
}

static LogModule gMqttModule =
{
    .moduleId = OUTPUT_MQTT,
    .init = MqttInit,
    .output = MqttOutput,
    .deinit = MqttDeInit,
};

static void __attribute__ ((constructor)) MqttModuleRegister()
{
    PRINT("MqttModuleRegister\n");
    LogModuleRegister( &gMqttModule );
}
