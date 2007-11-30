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

//malloc((size_t) 1);

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




/* logging functions */
void logInfo(const char *msg, ...) {
	if (cs_config.loglevel < LOGINFO) 		// Do we need this loglevel?
		return;
	
	va_list ap, aq, ar; // We need three of these because we cannot re-use a va_list variable
	
	va_start(ap, msg);
	va_copy(aq, ap);
	va_copy(ar, ap);
	
	char prepend[] = "<timestamp> - <info>: ";
	char postpend[] = "\n";
	char output[strlen(prepend) + strlen(postpend) + strlen(msg) + 1];
	output[0] ='\0';
	
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,postpend);
	
	if (cs_config.infofd != NULL) {	// Do we want to log this to a file?
//		fprintf(cs_config.infofd, "<timestamp> - <info>: ");
		vfprintf(cs_config.infofd, output , ap);
//		fprintf(cs_config.infofd, "\n");
	}
	
	if (cs_config.use_stderr == true) {	// Do we want to log this to syslog?
//		fprintf(stderr, "<timestamp> - <info>: ");
		vfprintf(stderr, output, aq);
//		fprintf(stderr,"\n");
	}
		
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
	
	char prepend[] = "<timestamp> - <error>: ";
	char postpend[] = "\n";
	char output[strlen(prepend) + strlen(postpend) + strlen(msg) + 1];
	output[0] ='\0';
	
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,postpend);
	
	if (cs_config.errfd != NULL) {	// Do we want to log this to a file?
//		fprintf(cs_config.errfd, "<timestamp> - <error>: ");
		vfprintf(cs_config.errfd, output, ap);
//		if (errno)
//			vfprintf(cs_config.errfd, "(error was: %s)",strerror(errno));
//		fprintf(cs_config.errfd, "\n");
	}

	if (cs_config.use_stderr == true) {	// Do we want to log this to syslog?
//		fprintf(stderr, "<timestamp> - <error>: ");
		vfprintf(stderr, output, aq);
//		if (errno)
//			vfprintf(stderr, "(error was: %s)",strerror(errno));

//		fprintf(stderr,"\n");
	}
	
	if (cs_config.use_syslog == true) { 	// Do we want to log this to syslog?
		syslog(LOG_ERR, msg, ar);
//		if (errno)
//			syslog(LOG_ERR, strerror(errno));
	}
	
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

	char prepend[] = "<timestamp> - <debug>: ";
	char postpend[] = "\n";
	char output[strlen(prepend) + strlen(postpend) + strlen(msg) + 1];
	output[0] ='\0';
	
	strcat(output,prepend);
	strcat(output,msg);
	strcat(output,postpend);
	
	if (cs_config.debugfd != NULL) {	// Do we want to log this to a file?
//		fprintf(cs_config.debugfd, "<timestamp> - <debug>: ");
		vfprintf(cs_config.debugfd, output, ap);
//		fprintf(cs_config.debugfd, "\n");
	}
	
	if (cs_config.use_stderr == true) {	// Do we want to log this to stderr?
//		fprintf(stderr, "<timestamp> - <debug>: ");
		vfprintf(stderr, output, aq);
//		fprintf(stderr, "\n");
	}
	
	if (cs_config.use_syslog == true) 	// Do we want to log this to syslog?
		syslog(LOG_DEBUG, output, ar);

	va_end(ap);
	va_end(aq);
	va_end(ar);
}
