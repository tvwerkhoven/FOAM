/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */

#include "foam_ui_library.h"

config_t ui_config = {
	.loglevel = LOGDEBUG,
	.use_syslog = false,
};

int sendMsg(const int sock, const char *buf) {
	printf("sending, len: %d", (int) strlen(buf));
	return write(sock, buf, strlen(buf)); // TODO non blocking maken
}

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

	fprintf(stdout,"\n");
	
	if (ui_config.use_syslog == true) { 	// Do we want to log this to syslog?
		syslog(LOG_ERR, msg, aq);
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
