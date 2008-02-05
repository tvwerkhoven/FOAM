/*
	User Interface Library
*/

#ifndef FOAM_UI_LIBRARY
#define FOAM_UI_LIBRARY

// INCLUDES //
/************/

#include "foam_library.h"

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
int sockRead(const int sock, char *msg, fd_set *lfd_set);

// DEFINES //
/***********/

#endif /* FOAM_UI_LIBRARY */
