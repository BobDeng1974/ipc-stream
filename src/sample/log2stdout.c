// Last Update:2019-02-21 11:19:36
/**
 * @file log2stdout.c
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2019-02-20
 */

#include <stdio.h>
#include "dbg.h"

static int ConsoleOutput( char *log )
{
    if ( !log ) {
        return -1;
    }

    printf( "%s", log );

    return 0;
}

static LogModule gConsoleModule =
{
    .moduleId = OUTPUT_CONSOLE,
    .output = ConsoleOutput,
};

static void __attribute__ ((constructor)) ConsoleModuleRegister()
{
    PRINT("ConsoleModuleRegister\n");
    LogModuleRegister( &gConsoleModule );
}
