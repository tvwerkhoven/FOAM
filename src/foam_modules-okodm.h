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
 @file foam_modules-okodm.h
 @author @authortim
 @date 2008-07-15
 
 @brief This file contains prototypes for routines to drive a 37 actuator Okotech DM using PCI interface
 */

#ifndef FOAM_MODULES_OKODM
#define FOAM_MODULES_OKODM

// HEADERS //
/***********/

#include <gsl/gsl_vector.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

// DEFINES //
/***********/

#define FOAM_MODOKODM_MAXVOLT 255		//!< Maximum voltage to set to the PCI card

#ifdef FOAM_MODOKODM_ALONE
#define FOAM_DEBUG 1			//!< set to 1 for debugging, in that case this module compiles on its own
#endif

// DATATYPES //
/*************/

/*!
 @brief Metadata on DM operations, typically an Okotech DM using a PCI board
 
 This struct holds metadata on DM operations. The fields denoted '(user)' must
 be supplied in advance by the user, while '(mod)' will be filled in during
 operation.
 
 Note: To set maxvolt above 255, modify the FOAM_MODOKODM_MAXVOLT define. This define
 overrides maxvolt if it's greater than 255 for your safety. 
 */
typedef struct {
	int minvolt;		//!< (user) minimum voltage that can be applied to the mirror
	int midvolt;		//!< (user) mid voltage (i.e. 'flat')
	int maxvolt;		//!< (user) maximum voltage. note: this value is overridden by FOAM_MODOKODM_MAXVOLT if greater than 255!
	int nchan;			//!< (user) number of channels, including substrate (i.e. 38)
	int *addr;			//!< (mod) pointer to array storing hardware addresses
	int fd;				//!< (mod) fd used with a mirror (to open the port)
	char *port;			//!< (user) port to use (i.e. "/dev/port")
	int pcioffset;		//!< (user) pci offset to use (4 on 32 bit systems)
	int pcibase[4];		//!< (user) max 4 PCI base addresses
} mod_okodm_t;

// PUBLIC PROTOTYPES //
/*********************/

/*!
 @brief This function sets the DM to the values given by *ctrl to the DM at *dm
 
 The vector/array *ctrl should hold control values for the mirror which
 range from -1 to 1, the domain being linear (i.e. from 0.5 to 1 gives twice
 the stroke). Since the mirror is actually only linear in voltage^2, this
 routine maps [-1,1] to [0,255] appropriately.
 
 @param [in] *ctrl Holds the controls to send to the mirror in the -1 to 1 range
 @param [in] *dm DM configuration information, filled by okoInitDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int okoSetDM(gsl_vector_float *ctrl, mod_okodm_t *dm);

/*!
 @brief Resets all actuators on the DM to dm->minvolt
 
 @param [in] *dm DM configuration information, filled by okoInitDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int okoRstDM(mod_okodm_t *dm);

/*!
 @brief Sets all actuators on the substrate to a certain voltage.
 
 @param [in] volt The voltage to set on all actuators
 @param [in] *dm DM configuration information, filled by okoInitDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int okoSetAllDM(mod_okodm_t *dm, int volt);

/*!
 @brief Initialize the module (software and hardware)
 
 You need to call this function *before* any other function, otherwise it will
 not work. The struct pointed to by *dm must hold some information to 
 init the DM with, see mod_okodm_t.
 
 @param [in] *dm DM configuration information
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int okoInitDM(mod_okodm_t *dm);

/*!
 @brief Close the module (software and hardware)
 
 You need to call this function *after* the last DM command. This
 resets the mirror and closes the file descriptor.
 
 @param [in] *dm DM configuration information, filled by okoInitDM()
 @return EXIT_SUCCESS on success, EXIT_FAILURE otherwise.
 */
int okoCloseDM(mod_okodm_t *dm);

#endif //#ifndef FOAM_MODULES_OKODM
