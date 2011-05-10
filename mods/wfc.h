/*
 wfc.h -- a wavefront corrector base class
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_WFC_H
#define HAVE_WFC_H

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <gsl/gsl_vector.h>

#include "types.h"
#include "config.h"

#include "devices.h"

using namespace std;

const string wfc_type = "wfc";

/*!
 @brief Base wavefront corrector class. This will be overloaded with the specific WFC type
 */
class Wfc: public Device {
private:
	// Common Wfc settings
	gsl_vector_float *wfc_amp;					//!< (Requested) actuator amplitudes, should be between -1 and 1.

public:
	int nact;														//!< Numeber of actuators in this device
	gain_t gain;												//!< Operating gain for this device
	
	// To be implemented by derived classes:
	/*! @brief Set Wfc in specific state using custom gain
	 
	 @param [in] wfcamp Wfc amplitudes for each actuator [-1, 1]
	 @param [in] g Gain for this actuation
	 @param [in] block Block until Wfc is in requested position
	 */
	virtual int actuate(const gsl_vector_float *wfcamp, const gain_t g, const bool block=false) = 0;
	
	virtual int calibrate() = 0;						//!< Calibrate actuator
	
	// From Device::
	virtual void on_message(Connection *const conn, string line);
	
	virtual ~Wfc();
	//!< Constructor for derived WFCs (i.e. SimWfc)
	Wfc(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, const bool online=true);
};

#endif // HAVE_WFC_H

/*!
 \page dev_wfc Wavefront corrector (WFC) devices
 
 The Wfc class provides control for wavefront correctors.
 
 \section dev_wfc_der Derived classes
 - \subpage dev_wfc_sim "Simulated membrane deformable mirror"
 
 \section dev_wfs_more See also
 - \ref dev_wfs "Wavefront sensor devices"
 
*/
