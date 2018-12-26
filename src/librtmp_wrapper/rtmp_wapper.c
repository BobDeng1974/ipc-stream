// Last Update:2018-12-18 09:31:09
/**
 * @file rtmp_wapper.c
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-14
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
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

    ctx = RtmpPubNew( _url, _nTimeout, _nInputAudioType, _nOutputAudioType, _nTimePolic );
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
    static int nIsFirst = 1;

    if ( !_pConext || !_pData ) {
        LOGE("check param error\n");
        if ( pBuf ) {
            free( pBuf );
        }
        return -1;
    }

    if ( !pBuf ) {
        LOGE("malloc error\n");
        return -1;
    }

    memset( pBuf, 0, _nSize );
    memset( nalus, 0, sizeof(nalus) );
    ret = H264DecodeFrame( _pData, _nSize, nalus, &size );
    if ( ret != DECODE_OK ) {
        LOGE("H264DecodeFrame error, ret = %d\n", ret );
        goto err;
    }

    if ( size < 0 ) {
        LOGE("check size error\n");
        goto err;
    }

    for ( i=0; i<size; i++ ) {
        switch( pNalu->type ) {
        case NALU_TYPE_SPS:
            if ( pNalu->addr && pNalu->size > 0 && nIsFirst ) {
                LOGI("set sps\n");
                RtmpPubSetVideoTimebase( _pConext, _nPresentationTime );
                RtmpPubSetSps( _pConext, pNalu->addr, pNalu->size );
            } else {
                //LOGE("get sps error\n");
                //goto err;
            }
            break;
        case NALU_TYPE_PPS:
            if ( pNalu->addr && pNalu->size > 0 && nIsFirst ) {
                LOGI("set pps\n");
                RtmpPubSetPps( _pConext, pNalu->addr, pNalu->size );
                nIsFirst = 0;
            } else {
                //LOGE("get sps error\n");
                //goto err;
            }
            break;
        case NALU_TYPE_IDR:
        case NALU_TYPE_SLICE:
            //LOGI("pNalu->type = %d\n", pNalu->type );
            if ( pNalu->addr && pNalu->size > 0 ) {
                memcpy( pBuf, pNalu->addr, pNalu->size );
                pBuf += pNalu->size;
            } else {
                LOGE("get nalu error, pNalu->addr = %p, pNalu->size = %d\n", pNalu->addr, pNalu->size );
            }
            break;
        default:
            break;
        }
        pNalu++;
    }

    if ( size >= MAX_NALUS_PER_FRAME ) {
        LOGE("nalus over flow\n");
    }

    //LOGI("pBuf - pBufAddr = %ld\n", pBuf - pBufAddr);
    if ( _nIsKey ) {
        ret = RtmpPubSendVideoKeyframe( _pConext, pBufAddr, pBuf-pBufAddr, _nPresentationTime );
        if ( ret != 0 ) {
            LOGE("RtmpPubSendVideoKeyframe() error, ret = %d, errno = %d\n", ret, errno );
            goto err;
        }
    } else {
        ret = RtmpPubSendVideoInterframe( _pConext, pBufAddr, pBuf-pBufAddr, _nPresentationTime );
        if ( ret != 0 ) {
            LOGE("RtmpPubSendVideoInterframe() error, ret = %d, errno = %d\n", ret, errno );
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
        if ( pBuf ) {
            LOGE("check param error\n");
            free( pBuf );
        }
        return -1;
    }

    if ( !pBuf ) {
        LOGE("malloc error\n");
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

//    LOGI("pBuf - pBufAddr = %ld\n", pBuf - pBufAddr);
    ret = RtmpPubSendAudioFrame( _pConext, pBufAddr, pBuf - pBufAddr, _nPresentationTime );
    if ( ret < 0 ) {
        LOGE("RtmpPubSendAudioFrame() error, ret = %d\n", ret );
        return -1;
    }

    free( pBufAddr );
    return 0;
}


int RtmpConnect( RtmpPubContext * _pConext)
{
    return (RtmpPubConnect(_pConext) );
}

