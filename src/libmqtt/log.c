// Last Update:2018-12-27 09:51:22
/**
 * @file log.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-27
 */

#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"

static LogCallBack pLogCb;

void SetLogCallBack( LogCallBack cb )
{
    pLogCb = cb;
}

static int GetCurrentTime( char *now_time )
{
    time_t now;
    struct tm *tm_now = NULL;

    time(&now);
    tm_now = localtime(&now);
    strftime( now_time, 200, "%Y-%m-%d %H:%M:%S", tm_now);

    return(0);
}

int PrintLog( const char *file, const char *function, int line, const char *format, ...  )
{
    char buffer[BUFFER_SIZE] = { 0 };
    va_list arg;
    int len = 0;
    char now[200] = { 0 };

    if ( !pLogCb ) {
        return -1;
    }

    memset( now, 0, sizeof(now) );
    GetCurrentTime( now );
    len = sprintf( buffer, "[ %s ] ", now );
    len = sprintf( buffer+len, "[ %s %s +%d ] ", file, function, line );
    va_start( arg, format );
    vsprintf( buffer+strlen(buffer), format, arg );
    va_end( arg );

    pLogCb( buffer );

    return 0;
}

