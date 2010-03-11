/*
 foam.h -- main FOAM header file. defines FOAM framework 
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*! 
	@file foam.h
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
	@date 20091103 10:32

	@brief This is the main header file for FOAM.

*/

#ifndef HAVE_FOAM_H
#define HAVE_FOAM_H

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#ifndef _GNU_SOURCE				
#define _GNU_SOURCE				// for vasprintf / asprintf
#endif
#include <sys/socket.h>				// networking
#include <arpa/inet.h>				// networking
#include <sys/types.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <syslog.h> 				// used for syslogging
#include <stdarg.h>
#include <pthread.h> 				// threads
#include <signal.h> 				// signal handlers
#include <time.h> 					// needed by libevent/event.h
#include <fcntl.h>
#include <gsl/gsl_linalg.h> 		// this is for SVD / matrix datatype
#include <gsl/gsl_blas.h> 			// this is for SVD

#include <string>
#include <stdexcept>

#include "autoconfig.h"
#include "types.h"
#include "config.h"
#include "protocol.h"
#include "foamctrl.h"

typedef Protocol::Server::Connection Connection;

class FOAM {
private:
	static sigset_t signal_mask;
	string conffile;

protected:
	// Server network functions
	Protocol::Server protocol;
	foamctrl *ptc;
	foamcfg *cs_config;
	// Message output and logging 
	Io io;
	// Inter-thread communication
	pthread_mutex_t mode_mutex;
	pthread_cond_t mode_cond;
	static pthread_attr_t attr;
	
	// was: modMessage();
	int on_message();
	int on_connect();
	
	int showhelp(string topic, string rest);
	
public:
	// was: modInitModule(foamctrl *ptc, foamcfg *cs_config);
	FOAM::FOAM();
	// was: modStopModule(foamctrl *ptc);
	FOAM::~FOAM();
	
	// was: modClosedInit, Loop, Finish
	virtual int closedInit();
	virtual int closedLoop();
	virtual int closedFinish();
	
	// was: modOpenInit, Loop, Finish
	virtual int openInit();
	virtual int openLoop();
	virtual int openFinish();
	
	// was: modCalibrate
	virtual int calib();
};

#endif // HAVE_FOAM_H
