/*! 
	@file foam_cs_library.c
	@brief Library file for the Control Software
	@author @authortim
	@date November 13 2007

	This file contains things necessary to run the Control Software that are not related to adaptive optics itself. 
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
// !!!:tim:20080326 need to fix this somewhere neater:
struct event_base *sockbase;

static int formatLog(char *output, const char *prepend, const char *msg) {
	char timestr[9];
	time_t curtime;
	struct tm *loctime;

	curtime = time (NULL);
	loctime = localtime (&curtime);
	strftime (timestr, 9, "%H:%M:%S", loctime);

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
	
//	printf("0");
	va_list ap, aq, ar; 						// We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
//	printf("1");	
	formatLog(logmessage, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) {  			// Do we want to log this to a file?
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(cs_config.infofd, logmessage , ap);
		else
			vfprintf(cs_config.infofd, msg , ap);
	}
//	printf("2");
	if (cs_config.use_stdout == true) { 			// Do we want to log this to stdout
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(stdout, logmessage, aq);
		else
			vfprintf(stdout, msg, aq);
	}
//	printf("3");
	if (cs_config.use_syslog == true) 			// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
//	printf("4");
}

void logDebug(const int flag, const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 			// Do we need this loglevel?
		return;
	
												// We only print log messages every logfrac frames
	if (flag & LOG_SOMETIMES && (ptc.frames % ptc.logfrac) != 0)
		return;
	
	char logmessage[COMMANDLEN];
	
//	printf("0");
	va_list ap, aq, ar; 						// We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
//	printf("1");
	
	formatLog(logmessage, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) {  			// Do we want to log this to a file?
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(cs_config.debugfd, logmessage , ap);
		else
			vfprintf(cs_config.debugfd, msg , ap);
	}
//	printf("2");
	
	if (cs_config.use_stdout == true) { 			// Do we want to log this to stdout
		if (!(flag & LOG_NOFORMAT)) 
			vfprintf(stdout, logmessage, aq);
		else
			vfprintf(stdout, msg, aq);
	}
//	printf("3");
		
	if (cs_config.use_syslog == true) 			// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
//	printf("4");
}

