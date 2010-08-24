/*
 types.h -- custom FOAM datatypes
 Copyright (C) 2008--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
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
*/

#ifndef HAVE_TYPES_H
#define HAVE_TYPES_H

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
typedef struct coord_t {
	coord_t(int _x=0, int _y=0) : x(_x), y(_y) { }
	int x;			//!< x coordinate
	int y;			//!< y coordinate
} coord_t;

/*!
 @brief We use this to define floating point 2-vectors (displacements etc)
 */
typedef struct fcoord_t {
	fcoord_t(float _x=0, float _y=0) : x(_x), y(_y) { }
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

/*!
 @brief Stores the mode of the AO system.
 
 Different AO system modes. See also mode_listen(), mode_open(), mode_calib().
 */
typedef enum { // aomode_t
	AO_MODE_OPEN=0,		//!< Open loop mode
	AO_MODE_CLOSED,		//!< Closed loop mode
	AO_MODE_CAL,			//!< Calibration mode (in conjunction with calmode_t)
	AO_MODE_LISTEN,		//!< Listen mode (idle)
	AO_MODE_UNDEF,		//!< Undefined mode (default)
	AO_MODE_SHUTDOWN	//!< Set to this mode for the worker thread to finish
} aomode_t;

/*! 
 @brief This enum is used to distinguish between various datatypes for processing.
 
*/
typedef enum {
	DATA_INT8,			//!< ID for int8_t
	DATA_UINT8,			//!< ID for uint8_t
	DATA_INT16,			//!< ID for int16_t
	DATA_UINT16			//!< ID for uint16_t
} dtype_t;

#endif // HAVE_TYPES_H 
