// Last Update:2018-12-16 22:16:11
/**
 * @file rtmp_wapper.h
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-16
 */

#ifndef RTMP_WAPPER_H
#define RTMP_WAPPER_H


#include "rtmp_publish.h"

extern RtmpPubContext * RtmpNewContext( const char * _url, unsigned int _nTimeout,
                                 RtmpPubAudioType _nInputAudioType,
                                 RtmpPubAudioType _nOutputAudioType,
                                 RtmpPubTimeStampPolicy _nTimePolic);
extern int RtmpConnect( RtmpPubContext * _pConext);
extern int RtmpSendAudio( RtmpPubContext *_pConext, char *_pData,
                   unsigned int _nSize, unsigned int _nPresentationTime );
extern int RtmpSendVideo( RtmpPubContext *_pConext, char *_pData,
                   unsigned int _nSize, int _nIsKey, unsigned int _nPresentationTime );

#endif  /*RTMP_WAPPER_H*/
