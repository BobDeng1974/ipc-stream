// Last Update:2018-12-16 17:54:49
/**
 * @file tests.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-14
 */

#include <stdio.h>
#include "dbg_internal.h"
#include "h264_decode_test.h"
#include "adts_deocde_test.h"

int main()
{
    LOGI("enter all tests\n");
    H264DecodeTest();
    AdtsDecodeTest();
    return 0;
}

