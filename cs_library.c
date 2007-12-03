/* 
	Library file for the Control Software
	Tim van Werkhoven, November 13 2007
*/

#include "cs_library.h"

control_t ptc = { //!< Global struct to hold system characteristics and other data. Initialize with complete but minimal configuration
	.mode = AO_MODE_OPEN,
	.wfs_count = 0,
	.wfc_count = 0
};

config_t cs_config = { 
	.listenip = "0.0.0.0",
	.listenport = 10000,
	.infofd = NULL,
//	.infofile = NULL,
	.errfd = NULL,
//	.errfile = NULL,
	.debugfd = NULL,
//	.debugfile = NULL,
	.use_syslog = false,
	.syslog_prepend = "foam",
	.use_stderr = true,
	.loglevel = LOGDEBUG
};

static int formatLog(char **output, const char *prepend, const char *msg) {
	char *timestr;
	printUTC(&timestr); // fill timestr with date
	timestr[24] = '\0';

	// TODO: this is bad, it allocates memory every call...
	*output = malloc((strlen(timestr) + strlen(prepend) + 1 + strlen(msg) + 1) * sizeof (char));
	
	strcat(*output,timestr);
	strcat(*output,prepend);
	strcat(*output,msg);
	strcat(*output,"\n");
	
	return EXIT_SUCCESS;
}

/* logging functions */
void logInfo(const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	char *output;
	formatLog(&output, " <info>: ", msg);
	
	if (cs_config.infofd != NULL) // Do we want to log this to a file?
		vfprintf(cs_config.infofd, output , ap);
	
	if (cs_config.use_stderr == true) // Do we want to log this to syslog
		vfprintf(stderr, output, aq);
		
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
	
	char *output;
	formatLog(&output, " <error>: ", msg);

	
	if (cs_config.errfd != NULL)	// Do we want to log this to a file?
		vfprintf(cs_config.errfd, output, ap);

	if (cs_config.use_stderr == true) // Do we want to log this to syslog?
		vfprintf(stderr, output, aq);
	
	if (cs_config.use_syslog == true) // Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}

void logDebug(const char *msg, ...) {
	if (cs_config.loglevel < LOGDEBUG) 	// Do we need this loglevel?
		return;
		
	va_list ap, aq, ar;
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);

	char *output;
	formatLog(&output, " <debug>: ", msg);
	
	if (cs_config.debugfd != NULL) 	// Do we want to log this to a file?
		vfprintf(cs_config.debugfd, output, ap);
	
	if (cs_config.use_stderr == true)	// Do we want to log this to stderr?
		vfprintf(stderr, output, aq);

	
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, output, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}
