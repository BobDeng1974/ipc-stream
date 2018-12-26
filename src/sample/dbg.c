// Last Update:2018-12-25 20:47:16
/**
 * @file dbg.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-25
 */

#include <stdio.h>
#include <string.h>

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
//            printf("key : %s, value : %s\n", key, value );
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
