// Last Update:2018-12-14 19:09:55
/**
 * @file h264_decode.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-14
 */

#include <string.h>
#include <arpa/inet.h>
#include "dbg.h"
#include "h264_decode.h"

/*
 * the NAL start prefix code (it can also be 0x00000001, depends on the encoder implementation)
 * */
#define NALU_START_CODE (0x00000001)

static int H264DecodeNalue( char *_pData, int _nStartCodeLen, OUT NalUnit *_pNalus, int _nMax )
{
    static char *pLast = NULL;
    static int nIndex = 0;
    NalUnit *pNalu = _pNalus + nIndex;

    if ( !_pNalus ) {
        return DECODE_PARARM_ERROR;
    }

    pNalu->addr = _pData;
    pNalu->type = (*_pData) & 0x1F;
    if ( pLast && nIndex > 0 )
        ( _pNalus + nIndex - 1)->size = _pData - pLast - _nStartCodeLen;

    pLast = _pData;
    nIndex ++;
    if ( nIndex >= _nMax ) {
        return DECODE_BUF_OVERFLOW;
    }
    return nIndex;
}

int H264DecodeFrame( char *_pFrame, int _nLen, OUT NalUnit *_pNalus, int *_pSize )
{
    char *pStart = _pFrame, *pEnd = pStart + _nLen;
    unsigned int *pStartCode = NULL, ret = 0;

    if ( !_pFrame || _nLen <= 0 || !_pNalus || !_pSize ) {
        return DECODE_PARARM_ERROR;
    }

    while( pStart <= pEnd ) {
        pStartCode = (unsigned int *)pStart;
        if ( htonl(*pStartCode) == NALU_START_CODE ) {
            pStart += 4;// skip start code
            ret = H264DecodeNalue( pStart, 4, _pNalus, *_pSize );
            if ( ret < 0 ) {
                return DECODE_FRAME_FAIL;
            }
        } else if (( htonl(*pStartCode) >> 8 ) == NALU_START_CODE ) {
            pStart += 3;
            ret = H264DecodeNalue( pStart, 3,  _pNalus, *_pSize );
            if ( ret < 0 ) {
                return DECODE_FRAME_FAIL;
            }
        } else {
            pStart++;
        }
    }

    *_pSize = ret;

    return DECODE_OK;
}
