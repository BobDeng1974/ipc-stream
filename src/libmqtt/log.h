// Last Update:2018-12-27 11:00:03
/**
 * @file log.h
 * @brief 
 * @author liyq
 * @version 0.1.00
 * @date 2018-12-27
 */

#ifndef LOG_H
#define LOG_H

typedef void (*LogCallBack)( char *log );

#define LOG(args...) PrintLog( __FILE__, __FUNCTION__, __LINE__, args )
#define DBG_ERROR(args...) PrintLog(  __FILE__, __FUNCTION__, __LINE__, args )
#define DBG_LOG(args...) LOG(args)
#define BASIC() printf("[ %s %s() +%d] ", __FILE__, __FUNCTION__, __LINE__ )
#define LOGE LOG
#define LOGI LOG 
#define BUFFER_SIZE 1024

extern void SetLogCallBack( LogCallBack cb );
extern int PrintLog( const char *file, const char *function, int line, const char *format, ...  );

#endif  /*LOG_H*/

