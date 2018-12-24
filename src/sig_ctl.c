// Last Update:2018-12-24 17:51:29
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
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include "dbg.h"
#include "sig_ctl.h"

/* Local context for Net callbacks */
typedef enum {
    SOCK_BEGIN = 0,
    SOCK_CONN,
} NB_Stat;

#define MAX_BUFFER_SIZE 128
#define DEFAULT_CON_TIMEOUT_MS  5000
#define MAX_PACKET_ID 65535 /* according to the MQTT specification - do not change! */
#define KEEP_ALIVE_SEC 60
#define CLIENT_ID "wolfmqttclient"
#define CMD_TIMEOUT_MS 6000 
#define APP_NAME "mqtt-rtmp-stream"
#define PRINT_BUFFER_SIZE       80

/* Setup defaults */
#ifndef SOCKET_T
    #define SOCKET_T        int
#endif
#ifndef SOERROR_T
    #define SOERROR_T       int
#endif
#ifndef SELECT_FD
    #define SELECT_FD(fd)   ((fd) + 1)
#endif
#ifndef SOCKET_INVALID
    #define SOCKET_INVALID  ((SOCKET_T)0)
#endif
#ifndef SOCK_CONNECT
    #define SOCK_CONNECT    connect
#endif
#ifndef SOCK_SEND
    #define SOCK_SEND(s,b,l,f) send((s), (b), (size_t)(l), (f))
#endif
#ifndef SOCK_RECV
    #define SOCK_RECV(s,b,l,f) recv((s), (b), (size_t)(l), (f))
#endif
#ifndef SOCK_CLOSE
    #define SOCK_CLOSE      close
#endif
#ifndef SOCK_ADDR_IN
    #define SOCK_ADDR_IN    struct sockaddr_in
#endif
#ifdef SOCK_ADDRINFO
    #define SOCK_ADDRINFO   struct addrinfo
#endif


typedef struct _SocketContext {
    SOCKET_T fd;
    NB_Stat stat;
    SOCK_ADDR_IN addr;
#ifdef MICROCHIP_MPLAB_HARMONY
    word32 bytes;
#endif
} SocketContext;


static MqttMessageCb pMessageCb;
static int mStopRead = 0;
static int mPacketIdLast = 0;

static word16 MqttGetPacketId(void)
{
    mPacketIdLast = (mPacketIdLast >= MAX_PACKET_ID) ?
        1 : mPacketIdLast + 1;
    return (word16)mPacketIdLast;
}


static int MessageArrived(MqttClient *client, MqttMessage *msg, byte msg_new, byte msg_done)
{
    byte buf[PRINT_BUFFER_SIZE+1];
    word32 len;

    if (msg_new) {
        /* Determine min size to dump */
        len = msg->topic_name_len;
        if (len > PRINT_BUFFER_SIZE) {
            len = PRINT_BUFFER_SIZE;
        }
        XMEMCPY(buf, msg->topic_name, len);
        buf[len] = '\0'; /* Make sure its null terminated */

        /* Print incoming message */
        LOGI("MQTT Message: Topic %s, Qos %d, Len %u\n",
               buf, msg->qos, msg->total_len);

    }

    /* Print message payload */
    len = msg->buffer_len;
    if (len > PRINT_BUFFER_SIZE) {
        len = PRINT_BUFFER_SIZE;
    }
    XMEMCPY(buf, msg->buffer, len);
    buf[len] = '\0'; /* Make sure its null terminated */
    PRINTF("Payload (%d - %d): %s",
           msg->buffer_pos, msg->buffer_pos + len, buf);
    if ( pMessageCb ) {
        pMessageCb( (char *)buf, msg->total_len );
    }

    if (msg_done) {
        LOGI("MQTT Message: Done\n");
    }

    /* Return negative to terminate publish processing */
    return MQTT_CODE_SUCCESS;
}
#if 0
{
    return 0;
}
#endif

static int MqttDisconnectedCb(MqttClient* client, int error_code, void* ctx)
{
    (void)client;
    (void)ctx;

    LOGI("Disconnect (error %d)\n", error_code);
    return 0;
}

static void *MqttWaitMessageThread( void *arg )
{
    MqttContex *pContex = (MqttContex *)arg;
    int rc = 0;

    if ( !pContex ) {
        LOGE("check param error\n1");
        return NULL;
    }

    do {
        /* Try and read packet */
        rc = MqttClient_WaitMessage( &pContex->client, CMD_TIMEOUT_MS );

        /* check for test mode */
        if (mStopRead) {
            LOGI("MQTT Exiting...");
            break;
        } else if (rc == MQTT_CODE_ERROR_TIMEOUT) {
            /* Keep Alive */
            LOGI("Keep-alive timeout, sending ping\n");

            rc = MqttClient_Ping( &pContex->client);
            if (rc != MQTT_CODE_SUCCESS) { LOGE("MQTT Ping Keep Alive Error: %s (%d)\n",
                       MqttClient_ReturnCodeToString(rc), rc);
                break;
            }
        } else if (rc != MQTT_CODE_SUCCESS) {
            /* There was an error */
            LOGE("MQTT Message Wait: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
            break;
        }


    } while (1);

    return NULL;
}

#ifndef WOLFMQTT_NO_TIMEOUT
static void setup_timeout(struct timeval* tv, int timeout_ms)
{
    tv->tv_sec = timeout_ms / 1000;
    tv->tv_usec = (timeout_ms % 1000) * 1000;

    /* Make sure there is a minimum value specified */
    if (tv->tv_sec < 0 || (tv->tv_sec == 0 && tv->tv_usec <= 0)) {
        tv->tv_sec = 0;
        tv->tv_usec = 100;
    }
}

#ifdef WOLFMQTT_NONBLOCK
static void tcp_set_nonblocking(SOCKET_T* sockfd)
{
#ifdef USE_WINDOWS_API
    unsigned long blocking = 1;
    int ret = ioctlsocket(*sockfd, FIONBIO, &blocking);
    if (ret == SOCKET_ERROR)
        PRINTF("ioctlsocket failed!");
#else
    int flags = fcntl(*sockfd, F_GETFL, 0);
    if (flags < 0)
        PRINTF("fcntl get failed!");
    flags = fcntl(*sockfd, F_SETFL, flags | O_NONBLOCK);
    if (flags < 0)
        PRINTF("fcntl set failed!");
#endif
}
#endif /* WOLFMQTT_NONBLOCK */
#endif /* !WOLFMQTT_NO_TIMEOUT */




static int NetConnect(void *context, const char* host, word16 port,
    int timeout_ms)
{
    SocketContext *sock = (SocketContext*)context;
    int type = SOCK_STREAM;
    int rc = -1;
    SOERROR_T so_error = 0;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    /* Get address information for host and locate IPv4 */
    switch(sock->stat) {
        case SOCK_BEGIN:
        {
            XMEMSET(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;

            XMEMSET(&sock->addr, 0, sizeof(sock->addr));
            sock->addr.sin_family = AF_INET;

            rc = getaddrinfo(host, NULL, &hints, &result);
            if (rc >= 0 && result != NULL) {
                struct addrinfo* res = result;

                /* prefer ip4 addresses */
                while (res) {
                    if (res->ai_family == AF_INET) {
                        result = res;
                        break;
                    }
                    res = res->ai_next;
                }

                if (result->ai_family == AF_INET) {
                    sock->addr.sin_port = htons(port);
                    sock->addr.sin_family = AF_INET;
                    sock->addr.sin_addr =
                        ((SOCK_ADDR_IN*)(result->ai_addr))->sin_addr;
                }
                else {
                    rc = -1;
                }

                freeaddrinfo(result);
            }
            if (rc != 0)
                goto exit;

            /* Default to error */
            rc = -1;

            /* Create socket */
            sock->fd = socket(sock->addr.sin_family, type, 0);
            if (sock->fd == SOCKET_INVALID)
                goto exit;

            sock->stat = SOCK_CONN;

            FALL_THROUGH;
        }

        case SOCK_CONN:
        {
        #ifndef WOLFMQTT_NO_TIMEOUT
            fd_set fdset;
            struct timeval tv;

            /* Setup timeout and FD's */
            setup_timeout(&tv, timeout_ms);
            FD_ZERO(&fdset);
            FD_SET(sock->fd, &fdset);
        #endif /* !WOLFMQTT_NO_TIMEOUT */

        #if !defined(WOLFMQTT_NO_TIMEOUT) && defined(WOLFMQTT_NONBLOCK)
            /* Set socket as non-blocking */
            tcp_set_nonblocking(&sock->fd);
        #endif

            /* Start connect */
            rc = SOCK_CONNECT(sock->fd, (struct sockaddr*)&sock->addr, sizeof(sock->addr));
        #ifndef WOLFMQTT_NO_TIMEOUT
            /* Wait for connect */
            if (rc < 0 || select((int)SELECT_FD(sock->fd), NULL, &fdset, NULL, &tv) > 0)
        #else
            if (rc < 0)
        #endif /* !WOLFMQTT_NO_TIMEOUT */
            {
                /* Check for error */
                socklen_t len = sizeof(so_error);
                getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (so_error == 0) {
                    rc = 0; /* Success */
                }
            #if !defined(WOLFMQTT_NO_TIMEOUT) && defined(WOLFMQTT_NONBLOCK)
                else if (so_error == EINPROGRESS) {
                    rc = MQTT_CODE_CONTINUE;
                }
            #endif
            }
            break;
        }

        default:
            rc = -1;
    } /* switch */

    (void)timeout_ms;

exit:
    /* Show error */
    if (rc != 0) {
        PRINTF("NetConnect: Rc=%d, SoErr=%d", rc, so_error);
    }

    return rc;
}

#ifdef WOLFMQTT_SN
static int SN_NetConnect(void *context, const char* host, word16 port,
    int timeout_ms)
{
    SocketContext *sock = (SocketContext*)context;
    int type = SOCK_DGRAM;
    int rc;
    SOERROR_T so_error = 0;
    struct addrinfo *result = NULL;
    struct addrinfo hints;

    /* Get address information for host and locate IPv4 */
    XMEMSET(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */

    XMEMSET(&sock->addr, 0, sizeof(sock->addr));
    sock->addr.sin_family = AF_INET;

    rc = getaddrinfo(host, NULL, &hints, &result);
    if (rc >= 0 && result != NULL) {
        struct addrinfo* res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET) {
            sock->addr.sin_port = htons(port);
            sock->addr.sin_family = AF_INET;
            sock->addr.sin_addr =
                ((SOCK_ADDR_IN*)(result->ai_addr))->sin_addr;
        }
        else {
            rc = -1;
        }

        freeaddrinfo(result);
    }

    if (rc == 0) {

    /* Create the socket */
        sock->fd = socket(sock->addr.sin_family, type, 0);
        if (sock->fd == SOCKET_INVALID) {
            rc = -1;
        }
    }

    if (rc == 0)
    {
    #ifndef WOLFMQTT_NO_TIMEOUT
        fd_set fdset;
        struct timeval tv;

        /* Setup timeout and FD's */
        setup_timeout(&tv, timeout_ms);
        FD_ZERO(&fdset);
        FD_SET(sock->fd, &fdset);
    #else
        (void)timeout_ms;
    #endif /* !WOLFMQTT_NO_TIMEOUT */

        /* Start connect */
        rc = SOCK_CONNECT(sock->fd, (struct sockaddr*)&sock->addr, sizeof(sock->addr));
    }

    /* Show error */
    if (rc != 0) {
        SOCK_CLOSE(sock->fd);
        PRINTF("NetConnect: Rc=%d, SoErr=%d", rc, so_error);
    }

    return rc;
}
#endif

static int NetWrite(void *context, const byte* buf, int buf_len,
    int timeout_ms)
{
    SocketContext *sock = (SocketContext*)context;
    int rc;
    SOERROR_T so_error = 0;
#ifndef WOLFMQTT_NO_TIMEOUT
    struct timeval tv;
#endif

    if (context == NULL || buf == NULL || buf_len <= 0) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

#ifndef WOLFMQTT_NO_TIMEOUT
    /* Setup timeout */
    setup_timeout(&tv, timeout_ms);
    setsockopt(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
#endif

    rc = (int)SOCK_SEND(sock->fd, buf, buf_len, 0);
    if (rc == -1) {
        /* Get error */
        socklen_t len = sizeof(so_error);
        getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error == 0) {
            rc = 0; /* Handle signal */
        }
        else {
        #ifdef WOLFMQTT_NONBLOCK
            if (so_error == EWOULDBLOCK || so_error == EAGAIN) {
                return MQTT_CODE_CONTINUE;
            }
        #endif
            rc = MQTT_CODE_ERROR_NETWORK;
            PRINTF("NetWrite: Error %d", so_error);
        }
    }

    (void)timeout_ms;

    return rc;
}

static int NetRead_ex(void *context, byte* buf, int buf_len,
    int timeout_ms, byte peek)
{
    SocketContext *sock = (SocketContext*)context;
    int rc = -1, timeout = 0;
    SOERROR_T so_error = 0;
    int bytes = 0;
    int flags = 0;
#if !defined(WOLFMQTT_NO_TIMEOUT) && !defined(WOLFMQTT_NONBLOCK)
    fd_set recvfds;
    fd_set errfds;
    struct timeval tv;
#endif

    if (context == NULL || buf == NULL || buf_len <= 0) {
        return MQTT_CODE_ERROR_BAD_ARG;
    }

    if (peek == 1) {
        flags |= MSG_PEEK;
    }

#if !defined(WOLFMQTT_NO_TIMEOUT) && !defined(WOLFMQTT_NONBLOCK)
    /* Setup timeout and FD's */
    setup_timeout(&tv, timeout_ms);
    FD_ZERO(&recvfds);
    FD_SET(sock->fd, &recvfds);
    FD_ZERO(&errfds);
    FD_SET(sock->fd, &errfds);

    #ifdef WOLFMQTT_ENABLE_STDIN_CAP
        FD_SET(STDIN, &recvfds);
    #endif

#else
    (void)timeout_ms;
#endif /* !WOLFMQTT_NO_TIMEOUT && !WOLFMQTT_NONBLOCK */

    /* Loop until buf_len has been read, error or timeout */
    while (bytes < buf_len) {

    #if !defined(WOLFMQTT_NO_TIMEOUT) && !defined(WOLFMQTT_NONBLOCK)
        /* Wait for rx data to be available */
        rc = select((int)SELECT_FD(sock->fd), &recvfds, NULL, &errfds, &tv);
        if (rc > 0)
        {
            /* Check if rx or error */
            if (FD_ISSET(sock->fd, &recvfds)) {
    #endif /* !WOLFMQTT_NO_TIMEOUT && !WOLFMQTT_NONBLOCK */

                /* Try and read number of buf_len provided,
                    minus what's already been read */
                rc = (int)SOCK_RECV(sock->fd,
                               &buf[bytes],
                               buf_len - bytes,
                               flags);
                if (rc <= 0) {
                    rc = -1;
                    goto exit; /* Error */
                }
                else {
                    bytes += rc; /* Data */
                }

    #if !defined(WOLFMQTT_NO_TIMEOUT) && !defined(WOLFMQTT_NONBLOCK)
            }
        #ifdef WOLFMQTT_ENABLE_STDIN_CAP
            else if (FD_ISSET(STDIN, &recvfds)) {
                return MQTT_CODE_STDIN_WAKE;
            }
        #endif
            if (FD_ISSET(sock->fd, &errfds)) {
                rc = -1;
                break;
            }
        }
        else {
            timeout = 1;
            break; /* timeout or signal */
        }
    #else
        /* non-blocking should always exit loop */
        break;
    #endif /* !WOLFMQTT_NO_TIMEOUT && !WOLFMQTT_NONBLOCK */
    } /* while */

exit:

    if (rc == 0 && timeout) {
        rc = MQTT_CODE_ERROR_TIMEOUT;
    }
    else if (rc < 0) {
        /* Get error */
        socklen_t len = sizeof(so_error);
        getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &so_error, &len);

        if (so_error == 0) {
            rc = 0; /* Handle signal */
        }
        else {
        #ifdef WOLFMQTT_NONBLOCK
            if (so_error == EWOULDBLOCK || so_error == EAGAIN) {
                return MQTT_CODE_CONTINUE;
            }
        #endif
            rc = MQTT_CODE_ERROR_NETWORK;
            PRINTF("NetRead: Error %d", so_error);
        }
    }
    else {
        rc = bytes;
    }

    return rc;
}

static int NetRead(void *context, byte* buf, int buf_len, int timeout_ms)
{
    return NetRead_ex(context, buf, buf_len, timeout_ms, 0);
}

#ifdef WOLFMQTT_SN
static int NetPeek(void *context, byte* buf, int buf_len, int timeout_ms)
{
    return NetRead_ex(context, buf, buf_len, timeout_ms, 1);
}
#endif

static int NetDisconnect(void *context)
{
    SocketContext *sock = (SocketContext*)context;
    if (sock) {
        if (sock->fd != SOCKET_INVALID) {
            SOCK_CLOSE(sock->fd);
            sock->fd = -1;
        }

        sock->stat = SOCK_BEGIN;
    }
    return 0;
}


/* Public Functions */
int MqttClientNet_Init(MqttNet* net)
{
#if defined(USE_WINDOWS_API) && !defined(FREERTOS_TCP)
    WSADATA wsd;
    WSAStartup(0x0002, &wsd);
#endif

#ifdef MICROCHIP_MPLAB_HARMONY
    static IPV4_ADDR    dwLastIP[2] = { {-1}, {-1} };
    IPV4_ADDR           ipAddr;
    int Dummy;
    int nNets;
    int i;
    SYS_STATUS          stat;
    TCPIP_NET_HANDLE    netH;

    stat = TCPIP_STACK_Status(sysObj.tcpip);
    if (stat < 0) {
        return MQTT_CODE_CONTINUE;
    }

    nNets = TCPIP_STACK_NumberOfNetworksGet();
    for (i = 0; i < nNets; i++) {
        netH = TCPIP_STACK_IndexToNet(i);
        ipAddr.Val = TCPIP_STACK_NetAddress(netH);
        if (ipAddr.v[0] == 0) {
            return MQTT_CODE_CONTINUE;
        }
        if (dwLastIP[i].Val != ipAddr.Val) {
            dwLastIP[i].Val = ipAddr.Val;
            PRINTF("%s", TCPIP_STACK_NetNameGet(netH));
            PRINTF(" IP Address: ");
            PRINTF("%d.%d.%d.%d\n", ipAddr.v[0], ipAddr.v[1], ipAddr.v[2], ipAddr.v[3]);
        }
    }
#endif /* MICROCHIP_MPLAB_HARMONY */

    if (net) {
        XMEMSET(net, 0, sizeof(MqttNet));
        net->connect = NetConnect;
        net->read = NetRead;
        net->write = NetWrite;
        net->disconnect = NetDisconnect;
        net->context = (SocketContext *)WOLFMQTT_MALLOC(sizeof(SocketContext));
        if (net->context == NULL) {
            return MQTT_CODE_ERROR_MEMORY;
        }
        XMEMSET(net->context, 0, sizeof(SocketContext));

        ((SocketContext*)(net->context))->stat = SOCK_BEGIN;
    }

    return MQTT_CODE_SUCCESS;
}

#ifdef WOLFMQTT_SN
int SN_ClientNet_Init(MqttNet* net)
{
    if (net) {
        XMEMSET(net, 0, sizeof(MqttNet));
        net->connect = SN_NetConnect;
        net->read = NetRead;
        net->write = NetWrite;
        net->peek = NetPeek;
        net->disconnect = NetDisconnect;
        net->context = (SocketContext *)WOLFMQTT_MALLOC(sizeof(SocketContext));
        if (net->context == NULL) {
            return MQTT_CODE_ERROR_MEMORY;
        }
        XMEMSET(net->context, 0, sizeof(SocketContext));
        ((SocketContext*)(net->context))->stat = SOCK_BEGIN;

    #if 0 //TODO: multicast support
        net->multi_ctx = (SocketContext *)WOLFMQTT_MALLOC(sizeof(SocketContext));
        if (net->multi_ctx == NULL) {
            return MQTT_CODE_ERROR_MEMORY;
        }
        XMEMSET(net->multi_ctx, 0, sizeof(SocketContext));
        ((SocketContext*)(net->multi_ctx))->stat = SOCK_BEGIN;
    #endif
    }

    return MQTT_CODE_SUCCESS;
}
#endif

int MqttClientNet_DeInit(MqttNet* net)
{
    if (net) {
        if (net->context) {
            WOLFMQTT_FREE(net->context);
        }
        XMEMSET(net, 0, sizeof(MqttNet));
    }
    return 0;
}

MqttContex * MqttNewContex( char *_pClientId, MqttQoS _nQos, char *_pUserName,
                            char *_pPasswd, char *_pTopic, char *_pHost, int _nPort, MqttMessageCb _pCb )
{
    MqttContex *pContex = NULL;
    pthread_t thread = 0;
    int rc = MQTT_CODE_SUCCESS, i = 0;

    if ( !_pClientId || !_pTopic || !_pHost || !_pCb ) {
        LOGE("check param error\n");
        return NULL;
    }

    pMessageCb = _pCb;

    pContex = (MqttContex*) malloc ( sizeof(MqttContex) );
    if ( !pContex ) {
        free( pContex );
        LOGE("malloc error\n");
        return NULL;
    }

    memset( pContex, 0, sizeof(MqttContex) );

    pContex->qos = _nQos;
    pContex->pTopic = _pTopic;
    rc = MqttClientNet_Init( &pContex->net);
    LOGI("MQTT Net Init: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClientNet_Init error\n");
        goto err;
    }

    pContex->tx_buf = (byte*)WOLFMQTT_MALLOC(MAX_BUFFER_SIZE);
    pContex->rx_buf = (byte*)WOLFMQTT_MALLOC(MAX_BUFFER_SIZE);

    /* Initialize MqttClient structure */
    rc = MqttClient_Init( &pContex->client, &pContex->net,
        MessageArrived,
        pContex->tx_buf, MAX_BUFFER_SIZE,
        pContex->rx_buf, MAX_BUFFER_SIZE,
        CMD_TIMEOUT_MS );

    LOGI("MQTT Init: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClient_Init error\n");
        goto err;
    }
    /* The client.ctx will be stored in the cert callback ctx during
       MqttSocket_Connect for use by mqtt_tls_verify_cb */
    pContex->client.ctx = pContex;
    rc = MqttClient_SetDisconnectCallback( &pContex->client, MqttDisconnectedCb, NULL);
    if ( rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClient_SetDisconnectCallback error\n");
        goto err;
    }
    /* Connect to broker */
    rc = MqttClient_NetConnect( &pContex->client, _pHost, _nPort,
        DEFAULT_CON_TIMEOUT_MS, 0, NULL );

    LOGI("MQTT Socket Connect: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClient_NetConnect error\n");
        goto err;
    }

        /* Build connect packet */
    XMEMSET(&pContex->connect, 0, sizeof(MqttConnect));
    pContex->connect.keep_alive_sec = KEEP_ALIVE_SEC;
    pContex->connect.clean_session = 1;
    pContex->connect.client_id = CLIENT_ID;

    /* Optional authentication */
    pContex->connect.username = _pUserName;
    pContex->connect.password = _pPasswd;

    /* Send Connect and wait for Connect Ack */
    rc = MqttClient_Connect( &pContex->client, &pContex->connect);
    LOGI("MQTT Connect: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClient_Connect error\n");
        goto err;
    }

    /* Validate Connect Ack info */
    LOGI("MQTT Connect Ack: Return Code %u, Session Present %d\n",
           pContex->connect.ack.return_code,
           (pContex->connect.ack.flags &
            MQTT_CONNECT_ACK_FLAG_SESSION_PRESENT) ?
           1 : 0
          );

    /* Build list of topics */
    XMEMSET( &pContex->subscribe, 0, sizeof(MqttSubscribe) );
    i = 0;
    pContex->topics[i].topic_filter = _pTopic;
    pContex->topics[i].qos = _nQos;

    /* Subscribe Topic */
    pContex->subscribe.packet_id = MqttGetPacketId();
    pContex->subscribe.topic_count = sizeof(pContex->topics) / sizeof(MqttTopic);
    pContex->subscribe.topics = pContex->topics;
    rc = MqttClient_Subscribe( &pContex->client, &pContex->subscribe);
    LOGI("MQTT Subscribe: %s (%d)\n", MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClient_Subscribe error\n");
        goto err;
    }
    /* show subscribe results */
    for (i = 0; i < pContex->subscribe.topic_count; i++) {
        pContex->topic = &pContex->subscribe.topics[i];
        LOGI("Topic %s, Qos %u, Return Code %u\n",
               pContex->topic->topic_filter,
               pContex->topic->qos, pContex->topic->return_code);
    }

    pthread_create( &thread, NULL, MqttWaitMessageThread, pContex );


    return pContex;

err:
    free( pContex );
    return NULL;
}


void MqttDestroyContex( MqttContex *_pConext )
{
    MqttContex *pContex = NULL;
    int rc = 0;

    if ( _pConext ) {
        pContex = _pConext;
        if ( !pContex ) {
            LOGE("check param error\n");
            return;
        }
        /* Unsubscribe Topics */
        XMEMSET(&pContex->unsubscribe, 0, sizeof(MqttUnsubscribe));
        pContex->unsubscribe.packet_id = MqttGetPacketId();
        pContex->unsubscribe.topic_count = sizeof(pContex->topics) / sizeof(MqttTopic);
        pContex->unsubscribe.topics = pContex->topics;

        /* Unsubscribe Topics */
        rc = MqttClient_Unsubscribe(&pContex->client,
                                    &pContex->unsubscribe);

        LOGI("MQTT Unsubscribe: %s (%d)",
               MqttClient_ReturnCodeToString(rc), rc);
        if (rc != MQTT_CODE_SUCCESS) {
            LOGE("MqttClient_Unsubscribe() error\n");
        }
        /* Disconnect */
        rc = MqttClient_Disconnect_ex(&pContex->client,
                                      &pContex->disconnect);

        LOGI("MQTT Disconnect: %s (%d)",
               MqttClient_ReturnCodeToString(rc), rc);
        if (rc != MQTT_CODE_SUCCESS) {
            LOGE("MqttClient_Disconnect_ex error\n");
        }

        rc = MqttClient_NetDisconnect(&pContex->client);

        LOGI("MQTT Socket Disconnect: %s (%d)",
               MqttClient_ReturnCodeToString(rc), rc);


        /* Free resources */
        if (pContex->tx_buf) WOLFMQTT_FREE(pContex->tx_buf);
        if (pContex->rx_buf) WOLFMQTT_FREE(pContex->rx_buf);

        /* Cleanup network */
        MqttClientNet_DeInit(&pContex->net);

    }

}

int MqttSend( MqttContex *_pConext, char *_pMessage, int _nLen )
{
    int rc = 0;
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

    /* Publish Topic */
    XMEMSET( &pContex->publish, 0, sizeof(MqttPublish));
    pContex->publish.retain = 0;
    pContex->publish.qos = pContex->qos;
    pContex->publish.duplicate = 0;
    pContex->publish.topic_name = pContex->pTopic;
    pContex->publish.packet_id = MqttGetPacketId();
    pContex->publish.buffer = (byte*)_pMessage;
    pContex->publish.total_len = (word16)XSTRLEN(_pMessage);

    rc = MqttClient_Publish( &pContex->client, &pContex->publish );
    LOGE("MQTT Publish: Topic %s, %s (%d)", pContex->publish.topic_name,
        MqttClient_ReturnCodeToString(rc), rc);
    if (rc != MQTT_CODE_SUCCESS) {
        LOGE("MqttClient_Publish error\n");
        return -1;
    }

    return 0;
}


