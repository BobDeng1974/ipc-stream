// Last Update:2018-12-14 11:44:22
/**
 * @file h264_decode.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-14
 */

#include <string.h>
#include "h264_decode.h"

/*
 * the NAL start prefix code (it can also be 0x00000001, depends on the encoder implementation)
 * */
#define NALU_START_CODE_4BYTES (0x01000000)
#define NALU_START_CODE_3BYTES (0x010000)


static int H264DecodeNalue( char *_pData, OUT NalUnit *_pNalu, int _nMax )
{
    static char *pLast = NULL;
    NalUnit *pLastNalu = _pNalu - 1;
    static int nIndex = 0;

    if ( !_pNalu ) {
        return DECODE_PARARM_ERROR;
    }

    _pNalu->addr = _pData;
    _pNalu->type = (*_pData) & 0x1F;
    if ( pLast && pLastNalu )
        pLastNalu->size = _pData - pLast;

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
        if ( *pStartCode == NALU_START_CODE_4BYTES ) {
            pStart += 4;// skip start code
            ret = H264DecodeNalue( pStart, _pNalus, *_pSize );
            if ( ret < 0 ) {
                return DECODE_FRAME_FAIL;
            }
        } else if ( *pStartCode == NALU_START_CODE_3BYTES ) {
            pStart += 3;
            ret = H264DecodeNalue( pStart, _pNalus, *_pSize );
            if ( ret < 0 ) {
                return DECODE_FRAME_FAIL;
            }
        } else if ( pStart[3] != 0x00 ) {
            /*
             *  ----------------------------------------------------------------
             * | x1 | x2 | x3 | x4 ( not 0x00 & not 0x01 ) | x5 | x6 | x7 | ... |
             *  ----------------------------------------------------------------
             *
             * if the 4th byte is not 0x00 and not 0x01, so
             * 1. x2 x3 x4 x5
             * 2. x3 x4 x5 x6
             * 3. x4 x5 x6 x7
             * this 3 case can not be 00 00 00 01 or 00 00 01, it expect the x4 must been 0x00
             * so we can skip them
             */
            pStart += 4;
        } else if ( pStart[2] != 0x00 ) {
            pStart += 3;
        } else if ( pStart[1] != 0x00 ) {
            pStart += 2;
        } else {
            pStart += 1;
        }
    }

    *_pSize = ret;

    return DECODE_OK;
}
