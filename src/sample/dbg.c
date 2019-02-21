// Last Update:2019-02-21 11:33:43
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

#define BUFFER_SIZE 1024
#define DBG_PARAM_CHECK_ERROR() PRINT("param check error\n")
#define DBG_MALLOC_ERROR( ptr ) if ( !ptr) {\
    PRINT("malloc error\n"); \
    return -1; \
}
#define MEMSET( ptr ) memset( ptr, 0, sizeof(*(ptr)) )
#define MODULE_MAX 5
#define ARRSZ( arr ) sizeof(arr)/sizeof(arr[0])

typedef struct {
    int output;
    char *file;
    int showTime;
    unsigned level;
    int verbose;
    LogModule *module;
    LogModule *modules[MODULE_MAX];
    int index;
} Logger;

static Logger gLogger;

int LoggerInit( LogParam *param )
{
    if ( !param ) {
        DBG_PARAM_CHECK_ERROR();
        return -1;
    }

    gLogger.output = param->output;
    gLogger.showTime = param->showTime;
    gLogger.level = param->level;
    gLogger.verbose = param->verbose;
    if ( param->file ) {
        int len = strlen( param->file );
        gLogger.file = (char*)malloc( len );
        DBG_MALLOC_ERROR( gLogger.file );
        memcpy( gLogger.file, param->file, len );
    }

    int i = 0;
    for ( i=0; i<gLogger.index; i++ ) {
        if ( gLogger.modules[i]->moduleId == gLogger.output ) {
            gLogger.module = gLogger.modules[i];
        }
    }

    if ( !gLogger.module ) {
        PRINT("get active module error\n");
        return -1;
    }

    if ( gLogger.module->init )
        gLogger.module->init( param );

    return 0;
}

int GetTime( char *now_time )
{
    time_t now;
    struct tm *tm_now = NULL;

    time(&now);
    tm_now = localtime(&now);
    strftime( now_time, 200, "%Y-%m-%d %H:%M:%S", tm_now);

    return(0);
}

int Dbg( unsigned logLevel, const char *file,
         const char *function, int line, const char *format, ...  )
{
    char buffer[BUFFER_SIZE] = { 0 };
    va_list arg;
    int len = 0;

    if ( gLogger.showTime ) {
        char now[200] = { 0 };
        memset( now, 0, sizeof(now) );
        GetTime( now );
        len = sprintf( buffer, "[ %s ] ", now );
    }

    if ( gLogger.verbose ) {
        len = sprintf( buffer+len, "[ %s %s +%d ] ", file, function, line );
    }

    va_start( arg, format );
    vsprintf( buffer+strlen(buffer), format, arg );
    va_end( arg );

    if ( gLogger.module && gLogger.module->output ) {
        gLogger.module->output( buffer );
    }

    return 0;

}

int LogModuleRegister( LogModule *module )
{
    if ( !module ) {
        DBG_PARAM_CHECK_ERROR();
        return -1;
    }

    if ( gLogger.index > MODULE_MAX ) {
        PRINT("can't register module, reach max\n");
        return -1;
    }

    gLogger.modules[gLogger.index++] = module;

    return 0;
}


