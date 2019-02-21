// Last Update:2019-02-21 11:00:00
/**
 * @file dbg.h
 * @brief 
 * @author felix
 * @version 0.1.00
 * @date 2018-12-26
 */

#ifndef DBG_H
#define DBG_H

#include <stdio.h>

typedef enum {
    OUTPUT_NONE,
    OUTPUT_FILE,
    OUTPUT_SOCKET,
    OUTPUT_MQTT,
    OUTPUT_CONSOLE,
} OutputType;

enum {
    DBG_LEVEL_DEBUG,
    DBG_LEVEL_INFO,
    DBG_LEVEL_FATAL,
};

typedef struct {
    char *topic;
    char *broker;
    char *clientId;
    char *user;
    char *passwd;
    int port;
    int qos;
} MqttParam;

typedef struct {
    int output;
    char *file;
    int showTime;
    unsigned level;
    int verbose;
    MqttParam *mqttParam;
} LogParam;

typedef struct {
    int moduleId;
    int (*init)( void *arg );
    int (*deinit)();
    int (*output)( char *log );
} LogModule;

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K"

#define DBG_BASIC_INFO __FILE__, __FUNCTION__, __LINE__
#define LOGI(args...) Dbg( DBG_LEVEL_DEBUG, DBG_BASIC_INFO,  args )
#define LOGE(args...) Dbg( DBG_LEVEL_FATAL, DBG_BASIC_INFO, args )
#define PRINT(args...) printf("[ %s %s() +%d ] ", DBG_BASIC_INFO );printf(args)

extern int LoggerInit( LogParam *param );
extern int Dbg( unsigned logLevel,
                const char *file,
                const char *function,
                int line,
                const char *format, ...  
                );
extern int LogModuleRegister( LogModule *module );

#endif  /*DBG_H*/
