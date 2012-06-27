/*
 types.h -- custom FOAM datatypes
 Copyright (C) 2008--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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
 @file foamtypes.h
 @brief This file contains some datatypes used.
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
*/

#ifndef HAVE_FOAMTYPES_H
#define HAVE_FOAMTYPES_H

using namespace std;

// STRUCTS AND TYPES //
/*********************/

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

#endif // HAVE_TYPES_H 
