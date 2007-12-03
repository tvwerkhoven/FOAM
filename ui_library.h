/*
	User Interface Library
*/

#ifndef UI_LIBRARY
#define UI_LIBRARY

// INCLUDES //
/************/

#include "ao_library.h"

// STRUCTS ETC //
/***************/

typedef struct {
	level_t loglevel;
	bool use_syslog;
} config_t;

// PROTOTYPES //
/**************/

// int sendMsg(const int sock, const char *buf);
int initSockC(in_addr_t host, int port, fd_set *cfd_set);
int parseArgs(int argc, char *argv[], in_addr_t *host, int *port);
int sockGetActive(fd_set *cfd_set);
void logDebug(const char *msg, ...);
void logErr(const char *msg, ...);
void logInfo(const char *msg, ...);

// DEFINES //
/***********/

#define CRIT 0
#define ERR 1
#define INFO 2
#define DEBUG 3 // debug already taken ;(
#define LOG_LEVEL 3 // 0: critical errors, 1: errors and notices, 2: user commands & info 3: debug
					// NB: level 3 also includes things like delays to better see what happens
#define LOG_FD stdout // FD to log to
#define DEBUG_SLEEP 1000000 // usleep time for debugmode (typically about 1s = 1000000 usec)


#endif /* UI_LIBRARY */
