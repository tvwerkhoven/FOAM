#include "foam_ui_library.h"

config_t ui_config = {
	.loglevel = LOGDEBUG,
	.use_syslog = false,
};

/* logging functions */
void logInfo(const char *msg, ...) {
	if (ui_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq; // We need two of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	
	fprintf(stdout, "<timestamp> - <info>: ");
	vfprintf(stdout, msg, aq);
	fprintf(stdout,"\n");
		
	if (ui_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, aq);
	
	va_end(ap);
	va_end(aq);
}

/* logging functions */
void logDebug(const char *msg, ...) {
	if (ui_config.loglevel < LOGDEBUG) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq; // We need two of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	
	fprintf(stdout, "<timestamp> - <debug>: ");
	vfprintf(stdout, msg, aq);
	fprintf(stdout,"\n");
		
	if (ui_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, aq);
	
	va_end(ap);
	va_end(aq);
}

/* logging functions */
void logErr(const char *msg, ...) {
	if (ui_config.loglevel < LOGERR) 	// Do we need this loglevel?
		return;

	va_list ap, aq;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	
	fprintf(stdout, "<timestamp> - <error>: ");
	vfprintf(stdout, msg, aq);
//	if (errno) // TODO: this currently causes a segmentation fault
//		vfprintf(stderr, "(error was: %s)",strerror(errno));
	fprintf(stdout,"\n");
	
	if (ui_config.use_syslog == true) { 	// Do we want to log this to syslog?
		syslog(LOG_ERR, msg, aq);
//		if (errno)
//			syslog(LOG_ERR, strerror(errno));
	}
	
	va_end(ap);
	va_end(aq);
}

int sockRead(const int sock, char *msg, fd_set *lfd_set) {
	// TODO: this only accepts STATIC length messages! buffer overflow problem! December 3 2007 is it?
	int nbytes;
	size_t msglen = strlen(msg); // TODO: this goes wrong, socket broken, implement libevent asap
	
	nbytes = recvfrom(sock, msg, msglen, 0, 0, 0); // TODO: nodig? of recv genoeg?
	msg[nbytes] = '\0';
	
	if (nbytes == 0) { 			// EOF, close the socket! 
		close(sock);
		FD_CLR(sock, lfd_set);
		return nbytes;
	}
	
	else if (nbytes == -1) { 	// Error occured, exit and return EXIT_FAILURE
		// TODO: how do we solve this? Cant use logERR?
		printf("Error in recvfrom: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	else { 						// Data on the socket, read it
		return nbytes;
	}
}
