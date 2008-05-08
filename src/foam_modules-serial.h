/*! 
 @file foam_modules-serial.h
 @author @authortim
 @date May 05, 2008
 
 @brief This file contains prototyped routines to drive the serial port (i.e. for filterwheels)
 
 \section Info
 
 For info, look at the source, it isn't that complicated (really)
 
 This module can compile on its own\n
 <tt>gcc foam_modules-serial.c -Wall -std=c99 -DFOAM_MODSERIAL_ALONE=1</tt>
 
 \section Functions
 
 \li drvSetSerial() - Sets a value on a serial port
 
 \section Configuration
 
 This module only supports these configurations:
 \li \b FOAM_MODSERIAL_ALONE (*undef*), ifdef, this module compiles on its own
 \li \b FOAM_DEBUG (*undef*), ifdef, gives lowlevel debug information
 
 \section History
 
 \li 2008-04-02: init
 */

#ifndef FOAM_MODULES_SERIAL
#define FOAM_MODULES_SERIAL

#include <stdio.h>		// for stuff
#include <unistd.h>		// for close
#include <stdlib.h>		// has EXIT_SUCCESS / _FAILURE (0, 1)
#include <sys/types.h>	// for linux (open)
#include <sys/stat.h>	// for linux (open)
#include <fcntl.h>		// for fcntl
#include <errno.h>
#include <string.h>
//#include <string.h>
//#include <math.h>

#ifdef FOAM_MODSERIAL_ALONE
#define FOAM_DEBUG 1		//!< set to 1 for debugging, in that case this module compiles on its OwnerGrabButtonMask
#endif

/*!
 @brief This function writes a string to a port and returns the bytes written
 
 @param [in] *port The port to write to
 @param [in] *cmd The string to write to the port
 @return Number of bytes written on success, EXIT_FAILURE otherwise.
 */
int drvSetSerial(const char *port, const char *cmd);

#endif //#ifndef FOAM_MODULES_SERIAL
