/*
 Copyright (C) 2008 Tim van Werkhoven
 
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
 @file foam_modules-serial.h
 @author @authortim
 @date 2008-07-15
 
 @brief This file contains prototyped routines to drive the serial port (i.e. for filterwheels)
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
#define FOAM_DEBUG 1		//!< set to 1 for debugging, in that case this module compiles on its own
#endif

/*!
 @brief This function writes a string to a port and returns the bytes written
 
 @param [in] *port The port to write to, like '/dev/ttyS0'
 @param [in] *cmd The string to write to the port, such as 'XR0\r'
 @return Number of bytes written on success, EXIT_FAILURE otherwise.
 */
int drvSetSerial(const char *port, const char *cmd);

#endif //#ifndef FOAM_MODULES_SERIAL
