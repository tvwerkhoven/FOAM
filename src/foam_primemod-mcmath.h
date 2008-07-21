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
	@file foam_primemod-mcmath.h
	@author @authortim
	@date 2008-04-18 12:55

	@brief This is the prime module header file for 'mcmath', which holds some the definition of some
	datatypes that can vary from setup to setup. There are also some define statement to control
	some aspects of the framework in general.
*/

#ifndef FOAM_PRIME_MCMATH
#define FOAM_PRIME_MCMATH

// GENERAL //
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

#define FOAM_CONFIG_PRE "mcmath"	//!< used as prefix for datafiles etc.


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
	CAL_PINHOLE=0,	//!< determine reference shifts after inserting a pinhole
	CAL_INFL,		//!< determine the influence functions for each WFS-WFC pair
	CAL_LINTEST,	//!< linearity test for WFCs
	CAL_SUBAPSEL,		//!< For subaperture selection
	CAL_DARK,		//!< dark fielding
	CAL_FLAT,		//!< flat fielding
	CAL_DARKGAIN		//!< generate dark and gain only for the subapertures for fast dark/flat fielding
} calmode_t;

/*!
 @brief Enum for WFC types
 
 A list of all WFC's used in the system
 */
typedef enum { // axes_t
	WFC_TT=0,		//!< WFC Type for tip-tilt mirrors
	WFC_DM=1		//!< WFC type for deformable mirrors
} wfctype_t;

/*!
 @brief Enum for filterwheel types

 This enum includes all filterwheels present in the hardware system. 
 */
typedef enum { //filter_t
	FILT_PINHOLE=0,	//!< Pinhole used for subaperture selection (pinhole in front of SH lenslet array, after TT/DM)
    FILT_OPEN,      //!< Normal operations filter position
	FILT_CLOSED,	//!< Closed filter position
    FILT_TARGET     //!< A target for doing stuff
} filter_t;

// We always use config.h
#include "config.h"
// We always use the main library for datatypes etc
#include "foam_cs_library.h"

// ROUTINE PROTOTYPES //
/**********************/

// These *must* be defined in a prime module
int drvSetupHardware(control_t *ptc, aomode_t aomode, calmode_t calmode);
int drvSetActuator(control_t *ptc, int wfc);
int drvGetImg(control_t *ptc, int wfs);

// LIBRARIES //
/*************/

#ifdef FOAM_MCMATH_DISPLAY
// for displaying stuff (SDL)
#include "foam_modules-dispcommon.h"
#endif

// for image file I/O
#include "foam_modules-img.h"
// for calibrating the image lateron
#include "foam_modules-calib.h"
// for SH routines
#include "foam_modules-sh.h"
// for data logging
#include "foam_modules-log.h"

#ifndef FOAM_SIMHW
// for okodm things
#include "foam_modules-okodm.h"
// for itifg stuff
#include "foam_modules-itifg.h"
// for daq2k board stuff
#include "foam_modules-daq2k.h"
// for serial port stuff
#include "foam_modules-serial.h"
#endif

// These are McMath specific (for the time being)
int MMAvgFramesByte(control_t *ptc, gsl_matrix_float *output, wfs_t *wfs, int rounds);
int MMDarkFlatFullByte(wfs_t *wfs, mod_sh_track_t *shtrack);
int MMDarkFlatSubapByte(wfs_t *wfs, mod_sh_track_t *shtrack);


#endif // #ifndef FOAM_PRIME_MCMATH

