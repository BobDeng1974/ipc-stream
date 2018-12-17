// Last Update:2018-12-17 11:39:34
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
#include "MQTTLinux.h"
#include "MQTTClient.h"

#define LOGA_DEBUG 0
#define LOGA_INFO 1
#define assert(a, b, c, d) myassert(__FILE__, __LINE__, a, b, c, d)

void MyLog(int LOGA_level, char* format, ...)
{
    static char msg_buf[256];
    va_list args;
    struct timeb ts;

    struct tm *timeinfo;

    ftime(&ts);
    timeinfo = localtime(&ts.time);
    strftime(msg_buf, 80, "%Y%m%d %H%M%S", timeinfo);

    sprintf(&msg_buf[strlen(msg_buf)], ".%.3hu ", ts.millitm);

    va_start(args, format);
    vsnprintf(&msg_buf[strlen(msg_buf)], sizeof(msg_buf) - strlen(msg_buf), format, args);
    va_end(args);

    printf("%s\n", msg_buf);
    fflush(stdout);
}

void myassert(char* filename, int lineno, char* description, int value, char* format, ...)
{
    if (!value)
    {
        va_list args;

        MyLog(LOGA_INFO, "Assertion failed, file %s, line %d, description: %s\n", filename, lineno, description);

        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }
    else
        MyLog(LOGA_DEBUG, "Assertion succeeded, file %s, line %d, description: %s", filename, lineno, description);
}

int MqttSignal()
{
    char *host = "loalhost";
    int port = 8090, rc = 0;
    Network n;
    MQTTClient c;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    unsigned char buf[100];
    unsigned char readbuf[100];

    NetworkInit(&n);
    NetworkConnect( &n, host, port );
    MQTTClientInit(&c, &n, 1000, buf, 100, readbuf, 100);
    data.willFlag = 1;
    data.MQTTVersion = 4;
    data.clientID.cstring = "single-threaded-test";
    data.username.cstring = "testuser";
    data.password.cstring = "testpassword";
    data.keepAliveInterval = 20;
    data.cleansession = 1;
    data.will.message.cstring = "will message";
    data.will.qos = 1;
    data.will.retained = 0;
    data.will.topicName.cstring = "will topic";
    MyLog(LOGA_DEBUG, "Connecting");
    rc = MQTTConnect(&c, &data);
    assert("Good rc from connect", rc == SUCCESS, "rc was %d", rc);

    return 0;
}
