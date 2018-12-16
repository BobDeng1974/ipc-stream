// Last Update:2018-12-16 22:13:42
/**
 * @file rtmp_wapper.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-14
 */

#include <stdlib.h>
#include <string.h>
#include "dbg.h"
#include "h264_decode.h"
#include "rtmp_publish.h"
#include "adts.h"

#define MAX_NALUS_PER_FRAME 64
#define MAX_ADTS_PER_FRAME 128

RtmpPubContext * RtmpNewContext( const char * _url, unsigned int _nTimeout,
                                 RtmpPubAudioType _nInputAudioType,
                                 RtmpPubAudioType _nOutputAudioType,
                                 RtmpPubTimeStampPolicy _nTimePolic)
{
    RtmpPubContext *ctx = NULL;
    int ret = 0;

    if ( !_url ) {
        return NULL;
    }

    ctx = RtmpNewContext( _url, _nTimeout, _nInputAudioType, _nOutputAudioType, _nTimePolic );
    if ( !ctx ) {
        return NULL;
    }

    ret = RtmpPubInit( ctx );
    if ( ret != 0 ) {
        RtmpPubDel( ctx );
        return NULL;
    }

    return ctx;
}

int RtmpSendVideo( RtmpPubContext *_pConext, char *_pData,
                   unsigned int _nSize, int _nIsKey, unsigned int _nPresentationTime )
{
    NalUnit nalus[MAX_NALUS_PER_FRAME], *pNalu = nalus;
    int ret = 0, i = 0;
    int size = MAX_NALUS_PER_FRAME;
    char *pBuf = ( char *)malloc( _nSize ), *pBufAddr = pBuf;

    if ( !_pConext || !_pData ) {
        return -1;
    }

    if ( !pBuf ) {
        return -1;
    }

    memset( pBuf, 0, _nSize );
    memset( nalus, 0, sizeof(nalus) );
    ret = H264DecodeFrame( _pData, _nSize, nalus, &size );
    if ( ret != DECODE_OK ) {
        goto err;
    }
    for ( i=0; i<size; i++ ) {
        switch( pNalu->type ) {
        case NALU_TYPE_SPS:
            if ( pNalu->addr && pNalu->size > 0 ) {
                RtmpPubSetVideoTimebase( _pConext, _nPresentationTime );
                RtmpPubSetSps( _pConext, pNalu->addr, pNalu->size );
            } else {
                LOGE("get sps error\n");
                goto err;
            }
            break;
        case NALU_TYPE_PPS:
            if ( pNalu->addr && pNalu->size > 0 ) {
                RtmpPubSetPps( _pConext, pNalu->addr, pNalu->size );
            } else {
                LOGE("get sps error\n");
                goto err;
            }
            break;
        case NALU_TYPE_IDR:
        case NALU_TYPE_SLICE:
            if ( pNalu->addr && pNalu->size > 0 ) {
                memcpy( pBuf, pNalu->addr, pNalu->size );
            } else {
                LOGE("get nalu error\n");
            }
            break;
        default:
            break;
        }
    }

    if ( size >= MAX_NALUS_PER_FRAME ) {
        LOGE("nalus over flow\n");
    }

    if ( _nIsKey ) {
        ret = RtmpPubSendVideoKeyframe( _pConext, pBufAddr, pBuf-pBufAddr, _nPresentationTime );
        if ( ret != 0 ) {
            goto err;
        }
    } else {
        ret = RtmpPubSendVideoInterframe( _pConext, pBufAddr, pBuf-pBufAddr, _nPresentationTime );
        if ( ret != 0 ) {
            goto err;
        }
    }

    return 0;
err:
    free( pBufAddr );
    return -1;
}

int RtmpSendAudio( RtmpPubContext *_pConext, char *_pData,
                   unsigned int _nSize, unsigned int _nPresentationTime )
{
    static int nIsFirst = 1;
    int ret = 0, nSize = MAX_ADTS_PER_FRAME, i = 0;
    Adts adts[ MAX_ADTS_PER_FRAME ], *pAdts = adts;
    char *pBuf = (char *) malloc ( _nSize ),  *pBufAddr = NULL;;

    if ( !_pConext || !_pData || _nSize == 0 ) {
        return -1;
    }

    if ( !pBuf ) {
        return -1;
    }

    pBufAddr = pBuf;

    if ( nIsFirst ) {
        char audioSpecCfg[] = { 0x14, 0x10 };

        RtmpPubSetAudioTimebase( _pConext, _nPresentationTime );
        RtmpPubSetAac( _pConext, audioSpecCfg, sizeof(audioSpecCfg) );
        nIsFirst = 0;
    }
    memset( adts, 0, sizeof(adts) );
    ret = AacDecodeAdts( _pData, _nSize, adts, &nSize );
    if ( ret != ADTS_DECODE_OK ) {
        LOGI("adts decode fail, ret = %d\n", ret );
        return -1;
    }

    if ( nSize <= 0 || nSize >= MAX_ADTS_PER_FRAME ) {
        LOGI("check nSize error, nSize = %d\n", nSize );
        return -1;
    }

    memset( pBuf, 0, _nSize );
    for ( i=0; i<nSize; i++ ) {
        if ( pAdts->addr && pAdts->size > 0 ) {
            memcpy( pBuf, pAdts->addr, pAdts->size );
            pBuf += pAdts->size;
        } else {
            LOGE("found invalid adts!!!\n");
        }
        pAdts++;
    }

    ret = RtmpPubSendAudioFrame( _pConext, pBufAddr, pBuf - pBufAddr, _nPresentationTime );
    if ( ret < 0 ) {
        return -1;
    }

    free( pBufAddr );
    return 0;
}


int RtmpConnect( RtmpPubContext * _pConext)
{
    return (RtmpPubConnect(_pConext) );
}

