// Last Update:2018-12-16 20:41:28
/**
 * @file adts_deocde_test.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-16
 */

#include <stdlib.h>
#include <stdio.h>
#include "dbg.h"
#include "unit_test.h"
#include "adts.h"
#include "adts.c"

#define AAC_FILE "../src/tests/media/h265_aac_1_16000_a.aac"
#define AAC_BUFFER_MAX_SIZE 1024*100
#define AAC_FILE_SIZE 895211
#define MAX_ADTS_SIZE 64
#define ADTS_HEADER_LEN 9
#define MAX_BUF_LEN 1024

static char gbuffer[ MAX_BUF_LEN ];

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

char *AdtsDecodeNormalTest()
{
    FILE *fp = fopen( AAC_FILE, "r ") ;
    char *pBuf = NULL;
    size_t res = 0;
    int ret = 0;
    Adts adts[MAX_ADTS_SIZE], *pAdts = adts;
    int size = MAX_ADTS_SIZE;
    char firstPktFirt4Bytes[] = { 0x00, 0x4c, 0x61, 0x76 };
    char SecondPktFirt4Bytes[] = { 0x48, 0x33, 0x0f, 0x4d };

    memset( adts, 0, sizeof(adts) );
    ASSERT_NOT_EQUAL( fp, NULL ); 
    pBuf = ( char * ) malloc( AAC_BUFFER_MAX_SIZE );
    ASSERT_NOT_EQUAL( pBuf, NULL );
    res = fread( pBuf, 1, AAC_BUFFER_MAX_SIZE, fp );
    ASSERT_EQUAL( (int)res, AAC_FILE_SIZE );
    ret = AacDecodeAdts( pBuf, AAC_FILE_SIZE, adts, &size );
    ASSERT_EQUAL( ret, ADTS_DECODE_OK );
    ASSERT_EQUAL( (int)(size > 1), 1 );
    LOGI("size = %d\n", size );
    ASSERT_EQUAL( (int)(pAdts->size > 0), 1 );
    ASSERT_EQUAL( pAdts->size, 51 );
    ASSERT_MEM_EQUAL( pAdts->addr, firstPktFirt4Bytes, 4 );
    pAdts++;
    ASSERT_EQUAL( (int)(pAdts->size > 0), 1 );
    ASSERT_EQUAL( pAdts->size, 259 );
    ASSERT_MEM_EQUAL( pAdts->addr, SecondPktFirt4Bytes, 4 );

    free( pBuf );
    fclose( fp );
    return NULL;
}

static char *AllTests()
{
    RUN_TEST_CASE( AdtsDecodeNormalTest );

    return NULL;
}

void AdtsDecodeTest()
{
    char *res = AllTests();
    if ( res ) {
        printf("%s\n", res );
    } else {
        printf("[ AdtsDecodeTest ] test pass\n");
    }
}

