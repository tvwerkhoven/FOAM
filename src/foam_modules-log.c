/*
 Copyright (C) 2008 Tim van Werkhoven (tvwerkhoven@xs4all.nl)
 
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
 */
/*! 
 @file foam_modules-log.c
 @author @authortim
 @date 2008-07-21
 
 \section Info
 
 This module provides some functions to log data to files, besides the usual
 debug, info and error logging provided by the framework itself. This module
 is intended to be used for logging measurements, voltages and other 
 'real' data, as opposed to hardware initialization and operational information
 that is logged to the other files.
 
 Multiple 'log-sessions' can be handled at the same time, as each session has
 its own mod_log_t struct holding all relevant data. This module is not thread-safe
 in the sense that multiple threads logging to the same file might cause problems,
 since the logging functions are not atomic, the output might be stored interleaved.
 If using one mod_log_t struct per thread, or if you're careful about logging, there 
 is no problem.
 
 Once a session is initialized, one can use wrapper routines to log data,
 or one can directly access the mod_log_t.fd filedescriptor to log to. Both are fine,
 and the latter method might be more sensible in some cases.
 
 \section Functions
 
 \li logInit() Initialize a log-session,
 \li logGSLVecFloat() Log a GSL vector stored in float format,
 \li logVecFloat() Log a vector stored in float format,
 \li logPTC() Log the state of the AO system as stored in a control_t struct,
 \li logMsg() Log a literal string to a logfile,
 \li logFinish() Finish a log-session.
 
 */

// HEADERS //
/***********/

#include "foam_modules-log.h"

// ROUTINES //
/************/

int logInit(mod_log_t *log, control_t *ptc) {
	int i=0;
	char *file;
	
	time_t t = time(NULL);
	struct tm *localt = localtime(&t);
	
	// Construct filename
	asprintf(&file, "%s-%s", FOAM_CONFIG_PRE, log->fname);
	
	// Check if log->mode is valid (i.e. is a write mode)
	while (log_allmodes[i] != NULL) {
		if (strcmp(log_allmodes[i], log->mode) == 0) {
			// Success! Try to open the file
			log->fd = fopen(file, log->mode);
			if (!log->fd) {
				logWarn("Could not open logfile '%s': %s", file, strerror(errno));
				return EXIT_FAILURE;
			}
			// If we're here, file opening worked
			fprintf(log->fd, "%s Logging succesfully started at %s.\n", log->comm, asctime(localt));
			logInfo(0, "%s Logging to '%s' started.", log->fname, asctime(localt));
			
			// Log the AO system state for the record (if wanted)
			// !!!:tim:20080721 temp disabled, ptc is not filled at the time
			// logInit() is called, needs fixing
//			if (ptc != NULL)
//				logPTC(log, ptc, log->comm);
			
			return EXIT_SUCCESS;
		}
		
		i++;
	}
	
	logWarn("Could not open logfile '%s', mode '%s' unknown.", file, log->mode); 
	log->use = false;
	
	return EXIT_FAILURE;
}

void logMsg(mod_log_t *log, char *prep, char *msg, int newline) {
	if (!log->use || msg == NULL) // return immediately if we're not using this log session, or if there is nothing to log (shouldn't happen here :P)
		return;
	
	if (prep != NULL) fprintf(log->fd, "%s ", prep);
	
	fprintf(log->fd, "%s", msg);
	// Append a newline if requested
	if (newline == 1)
		fprintf(log->fd, "\n");
}

void logPTC(mod_log_t *log, control_t *ptc, char *prep) {
	if (!log->use) // return immediately if we're not using this log session
		return;
	
	int i;
	
	if (prep != NULL) fprintf(log->fd, "%s ", prep);
	
	// Log PTC data
	fprintf(log->fd, "AO state info. Mode: %d Cal: %d Frames: %ld %f #WFS %d #WFC %d #FW %d\n", \
			ptc->mode, ptc->calmode, ptc->frames, ptc->fps, ptc->wfs_count, \
			ptc->wfc_count, ptc->fw_count);
	
	// Log WFS data
	for (i=0; i<ptc->wfs_count; i++) {
		if (prep != NULL) fprintf(log->fd, "%s ", prep);
		fprintf(log->fd, "WFS %d Name: %s Res: %d %d bpp %d Fieldframes: %d Scandir %d\n", \
				ptc->wfs[i].id, ptc->wfs[i].name, ptc->wfs[i].res.x, ptc->wfs[i].res.y, \
				ptc->wfs[i].bpp, ptc->wfs[i].fieldframes, ptc->wfs[i].scandir);
	}

	// Log WFC data
	for (i=0; i<ptc->wfc_count; i++) {
		if (prep != NULL) fprintf(log->fd, "%s ", prep);
		fprintf(log->fd, "WFC %d Name: %s Nact: %d PID Gain: %f, %f, %f, Ctrl:", \
				ptc->wfc[i].id, ptc->wfc[i].name, ptc->wfc[i].nact, \
				ptc->wfc[i].gain.p, ptc->wfc[i].gain.i, ptc->wfc[i].gain.d);
		
		logGSLVecFloat(log, ptc->wfc[i].ctrl, NULL, 1);
	}
	
	// Log FW data
	for (i=0; i<ptc->wfc_count; i++) {
		if (prep != NULL) fprintf(log->fd, "%s ", prep);
		fprintf(log->fd, "FW %d Name: %s # Filters: %d Current: %d\n", \
				ptc->filter[i].id, ptc->filter[i].name, ptc->filter[i].nfilts, \
				ptc->filter[i].curfilt);
	}
	
	// Done
}

void logVecFloat(mod_log_t *log, float *vec, int nelem, char *prep, int newline) {
	if (!log->use || vec == NULL) // return immediately if we're not using this log session, or if there is nothing to log
		return;
	
	int i;
	
	if (prep != NULL)
		fprintf(log->fd, "%s ", prep);
	
	// Log all but last elements
	for (i=0; i < nelem-1; i++)
		fprintf(log->fd, FOAM_MODULES_LOG_FLT "%s", vec[i], log->sep);
	
	// Log last element without separator
	fprintf(log->fd, FOAM_MODULES_LOG_FLT, vec[i]);

	// Append a newline if requested
	if (newline == 1)
		fprintf(log->fd, "\n");
}

void logGSLVecFloat(mod_log_t *log, gsl_vector_float *vec, char *prep, int newline) {
	if (!log->use || vec == NULL) // return immediately if we're not using this log session, or if vec is not allocated yet
		return;

	int i;
	
	if (prep != NULL)
		fprintf(log->fd, "%s ", prep);
	
	// Log all but last elements
	for (i=0; i < vec->size-1; i++)
		fprintf(log->fd, FOAM_MODULES_LOG_FLT "%s", gsl_vector_float_get(vec, i), log->sep);
	
	// Log last element without separator
	fprintf(log->fd, FOAM_MODULES_LOG_FLT, gsl_vector_float_get(vec, i));
	
	// Append a newline if requested
	if (newline == 1)
		fprintf(log->fd, "\n");
	
}
			

int logFinish(mod_log_t *log) {
	time_t t = time(NULL);
	struct tm *localt = localtime(&t);

	// Finish logging, close the file. 
	fprintf(log->fd, "%s Logging succesfully stopped at %s.\n", log->comm, asctime(localt));
	
	if (fclose(log->fd)) {
		logWarn("Error closing logfile '%s': %s.", log->fname, strerror(errno));	
		return EXIT_FAILURE;
	}
	
	logInfo(0, "Logging to '%s' succesfully stopped.", log->fname);	
	return EXIT_SUCCESS;
}
