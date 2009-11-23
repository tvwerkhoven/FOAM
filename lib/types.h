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
 */
/*! 
 @file types.h
 @brief This file contains some datatypes used.
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-15 18:06
 
 This header file contains a lot of structs to hold data used in 
 FOAM. These include things like the state of the AO system (\c foamctrl), 
 as well as some structs to track network connections to the CS.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

// INCLUDES //
/************/
#include <gsl/gsl_linalg.h> 		// this is for SVD / matrix datatype
#include <pthread.h>
#include <string>
#include "autoconfig.h"

using namespace std;

// STRUCTS AND TYPES //
/*********************/

/*!
 @brief We use this to define integer 2-vectors (resolutions etc)
 */
typedef struct {
	int x;			//!< x coordinate
	int y;			//!< y coordinate
} coord_t;

/*!
 @brief We use this to define floating point 2-vectors (displacements etc)
 */
typedef struct {
	float x;		//!< x coordinate
	float y;		//!< y coordinate
} fcoord_t;

/*!
 @brief We use this to store gain information for WFC's
 */
typedef struct {
	float p;		//!< proportional gain
	float i;		//!< integral gain
	float d;		//!< differential gain
} gain_t;

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
	CAL_LINTEST,		//!< linearity test for WFCs
	CAL_DARK,
	CAL_DARKGAIN,
	CAL_SUBAPSEL,
	CAL_FLAT
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
 @brief Helper enum for WFC types
 
 This should be enough for now, but can be expanded to include other
 WFCs as well.
 */
typedef enum { // axes_t
	WFS_SH=0		//!< WFS Type for Shack-Hartmann
} wfstype_t;


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


/*!
 @brief Stores the mode of the AO system.
 */
typedef enum { // aomode_t
	AO_MODE_OPEN=0,	//!< Open loop mode
	AO_MODE_CLOSED, //!< Closed loop mode
	AO_MODE_CAL, 	//!< Calibration mode (in conjunction with calmode_t)
	AO_MODE_LISTEN,	//!< Listen mode (idle)
	AO_MODE_SHUTDOWN //!< Set to this mode for the worker thread to finish
} aomode_t;

/*! 
 @brief This enum is used to distinguish between various datatypes for processing.
 
 Instead of using bpp or something else, this more general datatype identification
 also allows identification of foreign datatypes like a GSL matrix or non-integer
 datatypes (which is hard to distinguish between if only using bpp) like floats.
 
 It is used by functions that accept multiple datatypes, or will be accepting this
 in later versions. This way, routines can work on uint8_t data as well as on
 uint16_t data. The problem arises from the fact that cameras give different
 bitdepth outputs, meaning that routines working on this output need to be able
 to cope with different datatypes.
*/
typedef enum {
	DATA_INT8,			//!< ID for int8_t
	DATA_UINT8,			//!< ID for uint8_t
	DATA_INT16,			//!< ID for int16_t
	DATA_UINT16,		//!< ID for uint16_t
	DATA_GSL_M_F,		//!< ID for gsl_matrix_float
	DATA_GSL_V_F		//!< ID for gsl_vector_float
} dtype_t;

/*!
 @brief AO scanning mode enum
 
 This is used to distinguish between different AO modes. Typically, AO
 corrects both in X and Y direction, but in certain cases it might be
 useful to work only in one of the two, where only contrast in one
 direction is available (i.e. solar limb) as opposed to both directions
 (i.e. sunspot or planet).
 */
typedef enum { // axes_t
	AO_AXES_XY=0,		//!< Scan in X and Y direction
	AO_AXES_X,		//!< Scan X direction only
	AO_AXES_Y		//!< Scan Y direction only
} axes_t;

#endif /* __TYPES_H__ */
