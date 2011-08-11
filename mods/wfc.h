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
 
 \section wfc_netio Network IO
 
 Valid commends include:
 - set gain \<p\> \<i\> \<d\>: change PID gain for WFC
 - get gain: return current gain
 
 \section wfc_cfg Configuration parameters
 
 - none

 */
class Wfc: public foam::Device {
private:
	int nact;														//!< Number of actuators in this device
	
public:
	// Common Wfc settings
	typedef struct wfc_ctrl {
		wfc_ctrl(): target(NULL), err(NULL), prev(NULL), pid_int(NULL) { }
		gsl_vector_float *target;					//!< (Requested) actuator amplitudes, should be between -1 and 1.
		gsl_vector_float *err;						//!< Error between current and target actuation
		gsl_vector_float *prev;						//!< Previous actuator amplitudes
		gain_t gain;											//!< Operating gain for this device
		gsl_vector_float *pid_int;				//!< Integral part of the PID gain
		float i_ran[2];										//!< Range for individual pid_int elements
	} wfc_ctrl_t;
	
	wfc_ctrl_t ctrlparams;
	
	void set_nact(const int val) { nact = val; }
	int get_nact() const { return nact; }
	
	void set_gain(const double p, const double i, const double d) { ctrlparams.gain.p = p; ctrlparams.gain.i = i; ctrlparams.gain.d = d; }
	
	/*! @brief Update WFC control
	 
	 @param [in] err Error between target and current signal
	 @param [in] g Gain for update
	 @param [in] retain Factor of old control vector to keep (default: 1.0)
	 */
	int update_control(const gsl_vector_float *const err, const gain_t g, const float retain=1.0);
	/*! @brief Update WFC control using default gain & retain
	 
	 @param [in] err Error between target and current signal
	 */
	int update_control(const gsl_vector_float *const err) { return update_control(err, ctrlparams.gain); }

	// To be implemented by derived classes:
	/*! @brief Actuate WFC using internal control vector
	 
	 @param [in] block Block until WFC is in requested position (not always available)
	 */
	virtual int actuate(const bool block=false) = 0;
		
	virtual int calibrate();						//!< Calibrate actuator
	
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
 - \subpage dev_wfc_simulwfc "Simulate wavefront corrector"
 
 \section dev_wfs_more See also
 - \ref dev_wfs "Wavefront sensor devices"


*/
