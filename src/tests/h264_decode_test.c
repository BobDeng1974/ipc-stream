// Last Update:2018-12-26 09:59:09
/**
 * @file h264_decode_test.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-14
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "dbg.h"
#include "h264_decode.h"
#include "unit_test.h"
#include "h264_decode_test.h"

#define H264_FILE "../src/tests/media/len.h264"
#define MAX_BUF_LEN 1024
#define MAX_NALUS 64
#define VIDEO_BUF_LEN 10*1024*1024

static char gbuffer[ MAX_BUF_LEN ];

#include "../librtmp_wrapper/h264_decode.c"

static void DumpBuffer( char *_pBufName, char *_pBuf, int _nLen, int _nLine )
{
    unsigned char *pBuf = (unsigned char *)_pBuf;
    int i = 0;

    printf("[ %02d ] the buffer : %s is : \n", _nLine,  _pBufName );
    for ( i=0; i<_nLen; i++ ) {
        printf("0x%02x, ", pBuf[i] );
    }
    printf("\n");
}

static char *H264DecodeNormalTest()
{
    FILE *fp = fopen( H264_FILE, "r" );
    char *pBuf = NULL, *pBufAddr = NULL;
    size_t res = 0;
    int keyFrameSize = 0, ret = 0, size = MAX_NALUS, i = 0, interFrameSize = 0;
    int foundSps = 0, foundPps = 0, foundSEI = 0, foundIDR = 0, foundSLICE = 0;
    NalUnit nalus[MAX_NALUS], *pNalues = nalus;
    char spsFirst4Bytes[] = { 0x67, 0x42, 0xc0, 0x15 };
    char ppsFirst4Bytes[] = { 0x68, 0xcb, 0x83, 0xcb};
    char keyFrameSEIfirst4Bytes[] = { 0x06, 0x05, 0xff, 0xff };
    char keyFrameIDRfirst4Bytes[] = { 0x65, 0x88, 0x84, 0x08 };
    char interFrameSLICEFirst4Bytes[] = { 0x41,  0x9a, 0x38, 0x11 };
    
    memset( pNalues, 0, sizeof(nalus) );
    ASSERT_NOT_EQUAL( fp, NULL );
    pBuf = ( char *) malloc( VIDEO_BUF_LEN );
    pBufAddr = pBuf;
    memset( pBuf, 0, VIDEO_BUF_LEN );
    ASSERT_NOT_EQUAL( pBuf, NULL );
    res = fread( pBuf, 1, VIDEO_BUF_LEN, fp );
    if ( res != VIDEO_BUF_LEN ) {
        LOGI("errno = %d, res = %d\n", errno, (int)res );
    }
    ASSERT_EQUAL( (int)res, 3505414 );
    keyFrameSize = *(int *)pBuf;
    pBuf += 4;
    ASSERT_EQUAL( keyFrameSize, 28619 );
    ret = H264DecodeFrame( pBuf, keyFrameSize, nalus, &size );
    ASSERT_EQUAL( ret, DECODE_OK );
    LOGI("size = %d\n", size );
    ASSERT_EQUAL( (int)(size >= 0), 1 );
    for ( i=0; i<size; i++ ) {
        switch( pNalues->type ) {
        case NALU_TYPE_SPS:
            LOGI("get NALU_TYPE_SPS\n");
            foundSps = 1;
            ASSERT_MEM_EQUAL( spsFirst4Bytes, pNalues->addr, 4 );
            ASSERT_EQUAL( pNalues->size, 28 );
            break;
        case NALU_TYPE_PPS:
            LOGI("get NALU_TYPE_PPS\n");
            foundPps = 1;
            ASSERT_MEM_EQUAL( ppsFirst4Bytes, pNalues->addr, 4 );
            ASSERT_EQUAL( pNalues->size, 5 );
            break;
            break;
        case NALU_TYPE_SEI:
            LOGI("get NALU_TYPE_SEI\n");
            ASSERT_MEM_EQUAL( keyFrameSEIfirst4Bytes, pNalues->addr, 4 );
            ASSERT_EQUAL( pNalues->size, 625 );
            foundSEI = 1;
            break;
        case NALU_TYPE_IDR:
            LOGI("get NALU_TYPE_IDR,pNalues->size = %d\n", pNalues->size );
            foundIDR = 1;
            ASSERT_EQUAL( (int)(pNalues->size > 0), 1 );
            ASSERT_MEM_EQUAL( keyFrameIDRfirst4Bytes, pNalues->addr, 4 );
            break;
        default:
            break;
        }
        pNalues++;
    }
    ASSERT_EQUAL( foundSps, 1 );
    ASSERT_EQUAL( foundPps, 1 );
    ASSERT_EQUAL( foundSEI, 1 );
    ASSERT_EQUAL( foundIDR, 1 );
    /* test non key frames */
    pBuf += keyFrameSize;
    interFrameSize = *(int *) pBuf;
    LOGI("interFrameSize = %d\n", interFrameSize );
    ASSERT_EQUAL( (int)(interFrameSize >0 ), 1 );
    pNalues = nalus;
    size = MAX_NALUS;
    ret = H264DecodeFrame( pBuf, interFrameSize, nalus, &size );
    ASSERT_EQUAL( ret, DECODE_OK );
    ASSERT_EQUAL( (int)(size>0), 1 );
    LOGI("size = %d\n", size );
    for ( i=0; i<size; i++ ) {
        static int first = 1;

        switch( pNalues->type) {
        case NALU_TYPE_IDR:
            LOGI("get NALU_TYPE_IDR\n");
            break;
        case NALU_TYPE_SLICE:
            if ( first ) {
                LOGI("get NALU_TYPE_SLICE\n");
                foundSLICE = 1;
                ASSERT_MEM_EQUAL( interFrameSLICEFirst4Bytes, pNalues->addr, 4 );
            }
            break;
        default:
            LOGI("get nalu type %d\n", pNalues->type );
            break;
        }
        pNalues++;
    }

    ASSERT_EQUAL( foundSLICE, 1 );

    fclose( fp );
    free( pBufAddr );
    return NULL;
}

static char *AllTests()
{
    RUN_TEST_CASE( H264DecodeNormalTest );

    return NULL;
}

void H264DecodeTest()
{
    char *res = AllTests();
    if ( res ) {
        printf("%s\n", res );
    } else {
        printf("[ H264DecodeTest ] test pass\n");
    }
}
