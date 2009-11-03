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
 @file foam_modules-log.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-21
 
 @brief This file contains prototyped routines to log measurements and other data
 */

#ifndef __LOG_H__
#define __LOG_H__

// LIBRARIES //
/*************/

#include <stdio.h>					// for stuff, fopen etc
#include <stdarg.h>
#include <time.h>					// for time and date functions
#include <gsl/gsl_linalg.h>			// for vectors and stuff
#include <stdbool.h>				// for true and false
#include <pthread.h> 				// threads
#include <syslog.h> 				// used for syslogging
#include "types.h"   		        // for custom datatypes


// DATATYPES //
/*************/

typedef struct {
	char *fname;		//!< (user) Filename for the logfile, will be appended to the #FOAM_CONFIG_PRE define
	FILE *fd;			//!< (foam) Filedescriptor to log to
	char *mode;			//!< (user) Mode to open the file with (append, truncate, etc)
	char *sep;			//!< (user) Value separator to use while logging (i.e. ", " for CSV)	
	char *comm;			//!< (user) Comment character, prepend before 'system messages', like 'log init' or 'log successful'
	bool use;			//!< (user) Toggle to use this log session or not. Can be used to temporarily stop logging.
} mod_log_t;

// GLOBALS / DEFINES //
/*********************/

// This is a little superfluous as only "r" is not allowed. However, this 
// is a somewhat cleaner approach as possible future extensions are also covered
// (i.e. disallowed) with this method.
static char *log_allmodes[] = {"r+", "w", "w+", "a", "a+", NULL}; //!< The allowed modes for mod_log_t.mode

// This define is used to print floating point numbers
#define FOAM_MODULES_LOG_FLT "%.8f" //!< Format string for floating point numbers

// PROTOTYPES //
/**************/

/*! @brief Initialize logging
 
 Initialize logging with the details stored in *log. This opens a file
 and an FD pointing to that file ready to write to it. Pass a pointer
 to the control_t struct used to issue a logPTC() after successful file opening
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @param [in] *ptc Pointer to control_t struct if a logPTC() is wanted, NULL otherwise
 @return EXIT_FAILURE upon error opening the file or EXIT_SUCCESS if fopen'ing worked 
 */
int logInit(mod_log_t *log, control_t *ptc);

/*! @brief Log a literal string to a logfile
 
 To log a message to a logfile, one can use this function. It does not format 
 the string *msg or *prep as printf functions do, it must be a literal string.
 If one wishes formatting, use fprintf(log->fd, ...) instead. The advantage of this
 function is that it checks log->use.
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @param [in] *prep A prefix used for logging, can 'ptc:' or '#' or even log->comm
 @param [in] *msg A literal string to log to the file
 @param [in] *app An appendix used after logging. Can be "\n" or " " etc. Default "\n" if NULL
*/ 
void logMsg(mod_log_t *log, char *prep, char *msg, char *app);

/*! @brief Reset logging, clearing the logfile
 
 This finished logging using logFinish(), and re-opens the logfile using
 logInit() with mode "w". This means all old data is removed and the logging
 is effectively reset.
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @param [in] *ptc Pointer to control_t struct if a logPTC() is wanted, NULL otherwise
 @return EXIT_FAILURE if logInit() or logFinish() fail, or EXIT_SUCCESS otherwise
 */
int logReset(mod_log_t *log, control_t *ptc);

/*! @brief Log the state of the AO system as stored in the control_t struct
 
 Log the main variables that define the state of the AO system. The logging is done over
 several lines. The first line contains some general system information, following that
 is a line for each WFS, after which each WFC configuration is listed on a line. Finally,
 each filterwheel is logged on a separate line as well. For details on what is logged,
 see a sample logfile or look at the code of the program.
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @param [in] *ptc Pointer to control_t struct that needs to be logged
 @param [in] *prep A prefix used for logging, can 'ptc:' or '#' or even log->comm
 */
void logPTC(mod_log_t *log, control_t *ptc, char *prep);

/*! @brief Log a vector stored in float format
 
 Log the values stored in a float vector to file, prepending the line with *prep.
 The values are stored in the format dictated by FOAM_MODULES_LOG_FLT.
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @param [in] *vec The vector of type float
 @param [in] nelem The number of elements in *vec
 @param [in] *prep A prefix used for logging, can 'ptc:' or '#' or even log->comm
 @param [in] *app An appendix used after logging. Can be "\n" or " " etc. Default "\n" if NULL
 */
void logVecFloat(mod_log_t *log, float *vec, int nelem, char *prep, char *app);

/*! @brief Log a GSL vector stored in float format
 Log the values stored in a GSL float vector to file, prepending the line with *prep.
 The values are stored in the format dictated by FOAM_MODULES_LOG_FLT.
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @param [in] *vec The vector of type gsl_vector_float
 @param [in] nelem Amount of elements to log, -1 for all. Useful if long vectors have a lot of trailing zeroes.
 @param [in] *prep A prefix used for logging, can 'ptc:' or '#' or even log->comm
 @param [in] *app An appendix used after logging. Can be "\n" or " " etc. Default "\n" if NULL
 */
void logGSLVecFloat(mod_log_t *log, gsl_vector_float *vec, int nelem, char *prep, char *app);

/*! @brief Finish logging, close the logfile
 
 Finish logging, write a last line to the logfile that logging will be stopped and
 close the file afterwards.
 
 @param [in] *log A mod_log_t struct filled with some configuration values
 @return EXIT_SUCCESS if closing the file went alright, EXIT_FAILURE otherwise.
 */
int logFinish(mod_log_t *log);


/*!
 @brief logInfo() prints out info messages to the appropriate streams.
 
 This function accepts a variable amount of arguments (like vfprintf) and passes
 them on to vfprintf more or less unchanged, except for the appended newline character
 and some prefix. This function returns immediately if the loglevel is too low to log
 info messages (see the level_t type for available levels).
 
 The parameter 'flag' can be set to a XOR of LOG_SOMETIMES and LOG_NOFORMAT.
 The first option makes sure that logging only happens every \a config.logfrac
 frames, which can be useful if logging during adaptive optics operations when
 logging too much can cause performance problems. LOG_NOFORMAT can be used to
 specify that logging should be done without formatting anything (i.e. no prefix
 nor a newline at the end of the log message).
 
 logInfo() first attempts to write to the file descriptor provided by \a cs_config.infofd
 if this is not \c NULL. After that the boolean variable \a cs_config.use_stderr
 is checked to see if the user requested output to stderr. Finally, 
 \a cs_config.use_syslog is checked to see if output to syslog is desired.
 This function does not report any problems at all.
 
 @param [in] flag Some options on how to log the data
 @param [in] msg The string to be passed on to vfprintf.
 */
void logInfo(const int flag, const char *msg, ...);

/*!
 @brief logErr() prints out error messages to the appropriate streams, and exits.
 
 This function assumes there was an error as it checks errno and appends 
 this to the error message. This function should be used for fatal errors, because
 this function exits after printing. See documentation on logInfo() for more information.
 
 @param [in] msg The string to be passed on to vfprintf.
 */
void logErr(const char *msg, ...);

/*!
 @brief logWarn() prints out error messages to the appropriate streams.
 
 This function assumes there was an error as it checks errno and appends 
 this to the error message. This function does not exit, and should be used
 for non-fatal errors. See documentation on logInfo() for more information.
 
 @param [in] msg The string to be passed on to vfprintf.
 */
void logWarn(const char *msg, ...);

/*!
 @brief logDebug() prints out debug messages to the appropriate streams.
 
 This function is used for debug logging. See documentation on logInfo() for more information.
 
 @param [in] flag Some options on how to log the data
 @param [in] msg The string to be passed on to vfprintf.
 */
void logDebug(const int flag, const char *msg, ...);

#endif // __LOG_H__
