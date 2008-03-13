/*! 
	@file foam_cs_library.c
	@brief Library file for the Control Software
	@author @authortim
	@date November 13 2007

	This file contains things necessary to run the Control Software that are not related to adaptive optics itself. 
*/

#include "foam_cs_library.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_OPEN,
	.calmode = CAL_INFL, 	// or CAL_PINHOLE
	.wfs_count = 0,
	.wfc_count = 0,
	.frames = 0,
	.filter = FILT_NORMAL
};

config_t cs_config = { 
	.listenip = "0.0.0.0", 	// listen on any IP by default, can be overridden by config file
	.listenport = 10000,	// listen on port 10000 by default
	.infofd = NULL,
	.errfd = NULL,
	.debugfd = NULL,
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stderr = true,
	.loglevel = LOGDEBUG,
	.logfrac = 1
};

conntrack_t clientlist;

static int formatLog(char *output, const char *prepend, const char *msg) {
	char timestr[9];
	time_t curtime;
	struct tm *loctime;

	curtime = time (NULL);
	loctime = localtime (&curtime);
	strftime (timestr, 9, "%H:%M:%S", loctime);

	output[0] = '\0'; // reset string
	
	strcat(output,timestr);
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,"\n");
	
	return EXIT_SUCCESS;
}

/* logging functions */
void logInfo(const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
		
	if (ptc.frames % cs_config.logfrac != 0)	// We only print log messages every logmod frames
		return;
	
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, logmessage , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to stderr
		vfprintf(stdout, logmessage, aq);
		
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

// const int LFLG_ALWAYS =
// const int LFLG_NOFORMAT = 


// void logInfo(int flag, const char *msg, ...) {
// 	
// }

void logDirect(const char *msg, ...) {
	// this log command is always logged and without any additional formatting on the loginfo level
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
		
	if (ptc.frames % cs_config.logfrac != 0)	// We only print log messages every logmod frames
		return;
				
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, msg , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to stderr
		vfprintf(stdout, msg, aq);
		
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_INFO, msg, ar);
	
	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logErr(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 	// Do we need this loglevel?
		return;

	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <error>: ", msg);

	
	if (cs_config.errfd != NULL)	// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stderr == true) // Do we want to log this to stderr?
		vfprintf(stdout, logmessage, aq);
	
	if (cs_config.use_syslog == true) // Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
	
	// There was an error, stop immediately
	exit(EXIT_FAILURE);
}

void logWarn(const char *msg, ...) {
	if (cs_config.loglevel < LOGERR) 	// Do we need this loglevel?
		return;

	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	formatLog(logmessage, " <warning>: ", msg);

	
	if (cs_config.errfd != NULL)	// Do we want to log this to a file?
		vfprintf(cs_config.errfd, logmessage, ap);

	if (cs_config.use_stderr == true) // Do we want to log this to stderr?
		vfprintf(stdout, logmessage, aq);
	
	if (cs_config.use_syslog == true) // Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
	
}

void logDebug(const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 	// Do we need this loglevel?
		return;
		
	if (ptc.frames % cs_config.logfrac != 0)	// We only print log messages every logmod frames
		return;
		
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);

	formatLog(logmessage, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) 	// Do we want to log this to a file?
		vfprintf(cs_config.debugfd, logmessage, ap);
	
	if (cs_config.use_stderr == true)	// Do we want to log this to stderr?
		vfprintf(stdout, logmessage, aq);

	
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}
