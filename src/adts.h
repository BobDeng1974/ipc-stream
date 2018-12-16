// Last Update:2018-12-16 17:34:00
/**
 * @file adts.h
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-16
 */

#ifndef ADTS_H
#define ADTS_H

typedef struct {
    char *addr;
    int size;
} Adts;

#define ADTS_DECODE_OK 0
#define ADTS_PARAM_ERROR -1
#define ADTS_OVERFLOW -2

extern int AacDecodeAdts( IN char *_pFrame, int _nLen, OUT Adts *_pAdts, int *_pSize );

#endif  /*ADTS_H*/
