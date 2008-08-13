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
	@file foam_cs_library.c
	@brief Library file for @name
	@author @authortim
	@date November 13 2007

	This file contains things necessary to run the @name that are 
	not related to adaptive optics itself. This includes things like networking,
	info/error logging, etc. 
 
	Note that 'cs' originated from 'control software', which was the name of 
	the software controlling AO before @name was thought of.
*/

#include "foam_cs_library.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_LISTEN,
	.calmode = CAL_INFL,
	.wfs_count = 0,
	.wfc_count = 0,
	.fw_count = 0,
	.frames = 0,
	.logfrac = 1000,
	.capped = 0,
};

config_t cs_config = { //!< Global struct to hold system configuration. Init with complete but minimal configuration
	.listenip = "0.0.0.0", 	// listen on any IP by default, can be overridden by config file
	.listenport = 10000,	// listen on port 10000 by default
	.infofile = NULL,
	.infofd = NULL,
	.errfile = NULL,
	.errfd = NULL,
	.debugfile = NULL,
	.debugfd = NULL,
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stdout = true,
	.loglevel = LOGDEBUG,
	.nthreads = 0
};

conntrack_t clientlist;
struct event_base *sockbase;

/*!
 @brief Internal function used to format log messages. Should not be used directly
 */
static int formatLog(char *output, const char *prepend, const char *msg) {
	char timestr[9];
	time_t curtime;
	struct tm *loctime;

	// get the current time and store it in 'timestr'
	curtime = time (NULL);
	loctime = localtime (&curtime);
	strftime (timestr, 9, "%H:%M:%S", loctime);

	// store the formatted log message to 'output'
	snprintf(output, (size_t) COMMANDLEN, "%s%s%s\n", timestr, prepend, msg);
	return EXIT_SUCCESS;
}

void logErr(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 			// Do we need this loglevel?
		return;

	char logmessage[COMMANDLEN];
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <ERROR>: ", msg);

	if (cs_config.errfd != NULL)				// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stdout == true) 			// Do we want to log this to stdout?
		vfprintf(stdout, logmessage, aq);
	
	if (cs_config.use_syslog == true) 			// Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
	
	// There was an error, stop immediately
	exit(EXIT_FAILURE);
}

void logWarn(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 			// Do we need this loglevel?
		return;

	char logmessage[COMMANDLEN];
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <WARNING>: ", msg);
	
	if (cs_config.errfd != NULL)				// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stdout == true) 			// Do we want to log this to stderr?
		vfprintf(stderr, logmessage, aq);
	
	if (cs_config.use_syslog == true) 			// Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
	
}

void logInfo(const int flag, const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 			// Do we need this loglevel?
		return;
	
												// We only print log messages every logfrac frames
	if (flag & LOG_SOMETIMES && (ptc.frames % ptc.logfrac) != 0)
		return;
	
	char logmessage[COMMANDLEN];
	
	va_list ap, aq, ar; 						// We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	// add prefix and newline to the message, i.e. format it nicely
	formatLog(logmessage, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) {  			// Do we want to log this to a file?
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(cs_config.infofd, logmessage , ap);
		else
			vfprintf(cs_config.infofd, msg , ap);
	}

	if (cs_config.use_stdout == true) { 			// Do we want to log this to stdout
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(stdout, logmessage, aq);
		else {
			vfprintf(stdout, msg, aq);
			fflush(stdout);
		}
	}

	if (cs_config.use_syslog == true) 			// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logDebug(const int flag, const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 			// Do we need this loglevel?
		return;
	
												// We only print log messages every logfrac frames
	if (flag & LOG_SOMETIMES && (ptc.frames % ptc.logfrac) != 0)
		return;
	
	char logmessage[COMMANDLEN];
	
	va_list ap, aq, ar; 						// We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) {  			// Do we want to log this to a file?
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(cs_config.debugfd, logmessage , ap);
		else
			vfprintf(cs_config.debugfd, msg , ap);
	}
	
	if (cs_config.use_stdout == true) { 			// Do we want to log this to stdout
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(stdout, logmessage, aq);
		else {
			vfprintf(stdout, msg, aq);
			fflush(stdout);
		}
	}
		
	if (cs_config.use_syslog == true) 			// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

