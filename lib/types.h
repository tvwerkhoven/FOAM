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
 @date 2008-07-15 18:06
 
 This header file contains a lot of structs to hold data used in 
 FOAM. These include things like the state of the AO system (\c foamctrl), 
 as well as some structs to track network connections to the CS.
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

/*!
 @brief Stores the mode of the AO system.
 */
typedef enum { // aomode_t
	AO_MODE_OPEN=0,	//!< Open loop mode
	AO_MODE_CLOSED, //!< Closed loop mode
	AO_MODE_CAL, 	//!< Calibration mode (in conjunction with calmode_t)
	AO_MODE_LISTEN,	//!< Listen mode (idle)
	AO_MODE_UNDEF,	//!< Undefined mode (default)
	AO_MODE_SHUTDOWN //!< Set to this mode for the worker thread to finish
} aomode_t;


#endif // HAVE_TYPES_H 
