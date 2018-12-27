// Last Update:2018-12-27 10:51:15
/**
 * @file func_trace.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-20
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    void *addr;
    int count;
} func_trace_t;

#ifdef DEBUG_TRACE_FUNCTION

static func_trace_t funcs[1024];

static int current = 0;

#define CHECK_EXIST() \
    for ( i=0; i<current; i++ ) { \
        if( funcs[i].addr == this_func ) { \
            found = 1; \
            break; \
        } \
    }

static void dump_funcs( FILE *fp )
{
    int i = 0;
    char buffer[128] = { 0 };

    if ( fp ) {
        sprintf( buffer, "current = %d\n", current );
        fwrite( buffer, 1, strlen(buffer), fp );
    }
    printf("\ncurrent = %d\n", current );
    for ( i=0; i<current; i++ ) {
        if ( funcs[i].count > 0 ) {
            printf("%p\n", funcs[i].addr );
            if ( fp ) {
                sprintf(buffer, "%p\n", funcs[i].addr );
                fwrite( buffer, 1, strlen(buffer), fp );
            }
        }
    }
}

static void __attribute__((__no_instrument_function__))
    __cyg_profile_func_enter(void *this_func, void *call_site)
{
    int i = 0, found = 0;

    CHECK_EXIST();
    if ( found ) {
        funcs[i].count++;
    } else {
        funcs[current].addr = this_func;
        funcs[current].count++;
        current++;
    }
}

static void __attribute__((__no_instrument_function__))
    __cyg_profile_func_exit(void *this_func, void *call_site)
{
    int i = 0,  found = 0;

    CHECK_EXIST();

    if ( found ) {
        funcs[i].count--;
    }
}

typedef struct {
    char *str;
    int sig;
} SigInfo;


#define ITEM_LIST \
    ADD_SIGNAL_ITEM( SIGHUP ) \
    ADD_SIGNAL_ITEM( SIGINT ) \
    ADD_SIGNAL_ITEM( SIGQUIT ) \
    ADD_SIGNAL_ITEM( SIGILL ) \
    ADD_SIGNAL_ITEM( SIGABRT ) \
    ADD_SIGNAL_ITEM( SIGFPE ) \
    ADD_SIGNAL_ITEM( SIGKILL ) \
    ADD_SIGNAL_ITEM( SIGSEGV ) \
    ADD_SIGNAL_ITEM( SIGPIPE ) \
    ADD_SIGNAL_ITEM( SIGALRM ) \
    ADD_SIGNAL_ITEM( SIGTERM ) \
    ADD_SIGNAL_ITEM( SIGUSR1 ) \
    ADD_SIGNAL_ITEM( SIGUSR2 ) \
    ADD_SIGNAL_ITEM( SIGCHLD ) \
    ADD_SIGNAL_ITEM( SIGCONT ) \
    ADD_SIGNAL_ITEM( SIGSTOP ) \
    ADD_SIGNAL_ITEM( SIGTTIN ) \
    ADD_SIGNAL_ITEM( SIGTTOU ) \
    ADD_SIGNAL_ITEM( SIGBUS ) \
    ADD_SIGNAL_ITEM( SIGPOLL ) \
    ADD_SIGNAL_ITEM( SIGPROF ) \
    ADD_SIGNAL_ITEM( SIGSYS ) \
    ADD_SIGNAL_ITEM( SIGTRAP ) \
    ADD_SIGNAL_ITEM( SIGURG ) \
    ADD_SIGNAL_ITEM( SIGVTALRM ) \
    ADD_SIGNAL_ITEM( SIGXCPU ) \
    ADD_SIGNAL_ITEM( SIGXFSZ ) \
    ADD_SIGNAL_ITEM( SIGIOT ) \
    ADD_SIGNAL_ITEM( SIGSTKFLT ) \
    ADD_SIGNAL_ITEM( SIGIO ) \
    ADD_SIGNAL_ITEM( SIGCLD ) \
    ADD_SIGNAL_ITEM( SIGPWR ) \
    ADD_SIGNAL_ITEM( SIGWINCH ) \
    ADD_SIGNAL_ITEM( SIGUNUSED ) 


#define ADD_SIGNAL_ITEM( item ) { #item, item },
#define ARRSZ(arr) (sizeof(arr)/sizeof(arr[0]))

static SigInfo gSignalList[] = 
{
    ITEM_LIST
};

static void SignalHandler( int sig )
{
    FILE *fp = fopen( "/tmp/oem/app/crash.log", "w+" );

    if ( fp && sig != SIGINT && sig != 17 && sig != 15 ) {
        char buffer[32] = { 0 };
        void *array[20];
        size_t size;
        char **strings;
        size_t i;
        char *pSigStr = NULL;

        for ( i=0; i<ARRSZ(gSignalList); i++ ) {
            if ( sig == gSignalList[i].sig ) {
                pSigStr = gSignalList[i].str;
            }
        }

        size = backtrace (array, 20);
        strings = backtrace_symbols (array, size);

        if( pSigStr ) {
            sprintf( buffer, "get sig %s\n", pSigStr );
        } else {
            sprintf( buffer, "get sig %d\n", sig );
        }
        fwrite( buffer, strlen(buffer), 1, fp );
        printf("%s\n", buffer );
        memset( buffer, 0, sizeof(buffer) );
        sprintf( buffer, "Obtained %zd stack frames.\n", size );
        printf("%s\n", buffer );
        fwrite( buffer, strlen(buffer), 1, fp );
        for ( i=0; i<size; i++ ) {
            printf("%s\n", strings[i] );
            fwrite( strings[i], strlen(strings[i]), 1, fp );
        }


        dump_funcs( fp );

        int mapfd = open ("/proc/self/maps", O_RDONLY);
        if (mapfd != -1)
        {
            fwrite( "\nMemory map:\n\n", 1, 14, fp );

            char buf[256];
            ssize_t n;

            while ((n = read (mapfd, buf, sizeof (buf)))) {
                printf("%s\n", buf );
                fwrite( buf, 1, n, fp );
            }

            close (mapfd);
        }

        fclose( fp );
    }

    switch( sig ) {
    case SIGINT:
    case SIGQUIT:
    case SIGABRT:
    case SIGSEGV:
    case SIGKILL:
    case SIGTERM:
        exit(0);
        break;

    }
}

static void __attribute__ ((constructor)) install_handler (void)
{
    int i = 0;

    printf("install signal handler\n");
    for ( i=0; i<32; i++ ) {
        signal( i, SignalHandler );
    }
}

#endif


