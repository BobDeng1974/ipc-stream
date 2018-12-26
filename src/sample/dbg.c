// Last Update:2018-12-26 17:31:17
/**
 * @file dbg.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-25
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dbg.h"

static Logger gLogger;

int FileOpen( char *_pLogFile )
{
    gLogger.fp = fopen( _pLogFile, "w+" );
    if ( !gLogger.fp ) {
        printf("open file %s error\n", _pLogFile );
        return -1;
    } else {
        printf("open file %s ok\n", _pLogFile);
    }


    return 0;
}

int WriteLog( char *log )
{
    size_t ret = 0;

    if ( !gLogger.fp ) {
        printf("check fd error\n");
        return -1;
    }

    ret = fwrite( log, strlen(log), 1, gLogger.fp );
    if ( ret != 1 ) {
        printf("error, ret != 1\n");
    } else {
    }
    fflush( gLogger.fp );

    return 0;
}

int LoggerInit( unsigned printTime, int output, char *pLogFile, int logVerbose )
{
    memset( &gLogger, 0, sizeof(gLogger) );

    gLogger.output = output;
    gLogger.logFile = pLogFile;
    gLogger.printTime = printTime;
    gLogger.logVerbose = logVerbose;

    switch( output ) {
    case OUTPUT_FILE:
        FileOpen( gLogger.logFile );
        break;
    case OUTPUT_SOCKET:
        break;
    case OUTPUT_MQTT:
        break;
    case OUTPUT_CONSOLE:
    default:
        break;
    }

    return 0;
}

int GetCurrentTime( char *now_time )
{
    time_t now;
    struct tm *tm_now = NULL;

    time(&now);
    tm_now = localtime(&now);
    strftime( now_time, 200, "%Y-%m-%d %H:%M:%S", tm_now);

    return(0);
}

int dbg( unsigned logLevel, const char *file, const char *function, int line, const char *format, ...  )
{
    char buffer[BUFFER_SIZE] = { 0 };
    va_list arg;
    int len = 0;
    char now[200] = { 0 };

    if ( gLogger.printTime ) {
        memset( now, 0, sizeof(now) );
        GetCurrentTime( now );
        len = sprintf( buffer, "[ %s ] ", now );
    }

    if ( gLogger.logVerbose ) {
        len = sprintf( buffer+len, "[ %s %s +%d ] ", file, function, line );
    }

    va_start( arg, format );
    vsprintf( buffer+strlen(buffer), format, arg );
    va_end( arg );

    switch( gLogger.output ) {
    case OUTPUT_FILE:
        WriteLog( buffer );
        break;
    case OUTPUT_SOCKET:
        break;
    case OUTPUT_MQTT:
        break;
    case OUTPUT_CONSOLE:
        if ( logLevel == DBG_LEVEL_FATAL ) {
            printf( RED"%s"NONE, buffer );
        } else {
            printf( GRAY"%s"NONE, buffer );
        }
        break;
    default:
        break;
    }

    return 0;

}

int DbgGetMemUsed( char *memUsed )
{
    char line[256] = { 0 }, key[32] = { 0 }, value[32] = { 0 };
    FILE *fp = NULL;
    char *ret = NULL;

    fp = fopen( "/proc/self/status", "r" );
    if ( !fp ) {
        printf("open /proc/self/status error\n" );
        return -1;
    }

    for (;;) {
        memset( line, 0, sizeof(line) );
        ret = fgets( line, sizeof(line), fp );
        if (ret) {
            sscanf( line, "%s %s", key, value );
            if (strcmp( key, "VmRSS:" ) == 0 ) {
                memcpy( memUsed, value, strlen(value) );
                fclose( fp );
                return 0;
            }
        }
    }

    fclose( fp );
    return -1;
}

