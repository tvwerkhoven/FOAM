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
	@file foam_primemod-dummy.h
	@author @authortim
	@date 2008-02-25

	@brief This is the prime mod header file for 'dummy', which holds the basic defines necessary
*/

// DEFINES //
/***********/

/*
 * These defines must be defined. Choose values as you like, but
 * don't touch them if you don't need to.
 */

#define FILENAMELEN 64				//!< maximum length for logfile names (no need to touch)
#define COMMANDLEN 1024				//!< maximum length for commands we read over the socket (no need to touch)

#define MAX_CLIENTS 8				//!< maximum number of clients that can connect
#define MAX_THREADS 4				//!< number of threads besides the main thread that can be created (unused atm)
#define MAX_FILTERS 8				//!< maximum number of filters one filterwheel can have

// DATATYPES //
/************/

/* 
 * These datatypes can be expanded as you like, but 
 * do not remove things that are already present!
 */

/*!
 @brief Helper enum for ao calibration mode operation.
 
 You can add your own calibration modes here which you can use to determine
 what kind of calibration a user wants.
 */
typedef enum { // calmode_t
	CAL_PINHOLE,	//!< determine reference shifts after inserting a pinhole
	CAL_INFL,		//!< determine the influence functions for each WFS-WFC pair
	CAL_LINTEST		//!< linearity test for WFCs
} calmode_t;

/*!
 @brief Helper enum for WFC types
 
 This should be enough for now, but can be expanded to include other
 WFCs as well.
 */
typedef enum { // axes_t
	WFC_TT=0,		//!< WFC Type for tip-tilt mirrors
	WFC_DM=1		//!< WFC type for deformable mirrors
} wfctype_t;

/*!
 @brief Helper enum for filterwheel types
 
 This should be enough for now, but can be expanded to include other
 filterwheels as well.
 */
typedef enum { // filter_t
	FILT_PINHOLE,	//!< Pinhole used for pinhole calibration
	FILT_OPEN,		//!< Open position, don't filter
	FILT_CLOSED		//!< Closed, don't let light through
} filter_t;

#include "config.h"
#include "foam_cs_library.h"

