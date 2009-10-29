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

/*! 
 @file foam_ui.c
 @author @authortim
 @date November 14 2007
 
 @brief This is a client which can connect to the Control Software
 
 At the moment this is not more than a collection of lines of code, it might
 not work (very well), use telnet instead.
 */

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <event.h>

void onRead() {
	printf("Reading...\n");
	sleep(1);
}

void onWrite() {
	printf("Writing...\n");
	sleep(1);
}

void onErr() {
	printf("Error...\n");
	sleep(1);
}

/*! 
 @brief main loop function
 
 Loop continuously with a pre-specified delay and get images and display these.
 */
int main (int argc, char *argv[]) {
	int sock;
	in_addr_t host;
	int port, delay, wfs;
	struct sockaddr_in serv_addr;
	char *buf;
	
	// argv[0]: program
	// argv[1]: delay (ms)
	// argv[2]: wfs # (0...n)
	// argv[3]: ip (default: 127.0.0.1 if ommitted)
	// argv[4]: port (default: 10000 if ommitted)
	
	if (argc < 3) {
		fprintf(stderr, "Error: not enough paramters, use this script as: <script> <delay (ms)> <wfs #> [ip] [port]\n");
		return -1;
	}
	else if (argc < 5) {
		printf("IP and port ommitted, using 127.0.0.1:10000.\n");
		port = 10000;
		host = inet_addr("127.0.0.1");
	}
	else {
		port = strtol(argv[4], NULL, 10);
		host = inet_addr(argv[3]);
		delay = strtol(argv[1], NULL, 10);
		wfs = strtol(argv[2], NULL, 10);
	}
	
	printf("Connecting to %d:%d using WFS %d every %d millisecond\n", host, port, wfs, delay);
	
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Socket error: %s", strerror(errno));
		return -1;
	}
	
	// Get the address fixed:
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = host;
	serv_addr.sin_port = htons(port);
	memset(serv_addr.sin_zero, '\0', sizeof(serv_addr.sin_zero));
	
	// Connect to the server
	if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
		fprintf(stderr, "connect() error: %s", strerror(errno));
		return -1;
	}
	
	// set up libevent
	// according to libevent docs:
	// event_base_new
	// bufferevent_new
	// bufferevent_base_set
	// bufferevent_enable
	
	struct event_base *evbase;		// Stores the eventbase to be used
	struct bufferevent *bufev;

	// init lib
	evbase = event_base_new();
	
	// make new buffered event
	bufev = bufferevent_new(sock, onRead, onWrite, onErr, NULL);
	
	// set to correct base
	if (bufferevent_base_set(evbase, bufev) == -1) {
		fprintf(stderr, "Error with bufferevent_base_set()\n");
		return -1;
	}

	// enable buffered event
	if (bufferevent_enable(bufev, EV_READ | EV_WRITE) == -1) {
		fprintf(stderr, "Error with bufferevent_enable()\n");
		return -1;
	}
	
	// dispatch loop
	event_base_dispatch(evbase);
		
	return 0;
}
