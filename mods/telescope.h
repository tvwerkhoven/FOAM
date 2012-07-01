/*
 telescope.h -- offload tip-tilt correction to some large-stroke device (telescope)
 Copyright (C) 2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef HAVE_TELESCOPE_H
#define HAVE_TELESCOPE_H

#include <gsl/gsl_vector.h>

#include "io.h"
#include "config.h"
#include "pthread++.h"
#include "types.h"

#include "devices.h"

using namespace std;

const string telescope_type = "telescope";

/*!
 @brief Telescope tracking controls
 
 Telescope (dev.telescope) can slew telescopes. This can be useful for 
 tip-tilt offloading.
 
 This class should convert shift in x,y in mm in some primary focus (Nasmyth, 
 Cassegrain) to a control command to drive the telescope tracking to this 
 position. Since the dimensions of these units are unknown (might be alt-az, 
 or anything else), we simply use dim0 and dim1 as dimensions. It is up to the 
 specific telescope implementation to give these dimensions and units meaning.
 
 Todo:
 
 - Use scalefacs, rotation and gain to convert pixel shift to intermediate focal plane shift
 - Implement specific conversion details from thereon in children (WHT, ...)

 \section telescope_method Method
 
 The Telescope class takes raw input (in any units), stored in c0, c1. This 
 is then scaled and rotated, and stored in sht0, sht1. A handler thread, 
 tel_handler(), then calls calc_offload() every X seconds. This routine converts
 the raw input shifts to generic coordaintes and then calls update_telescope_track()
 which should be implemented in a derived class to drive the telescope itself.
 
 (Telescope::set_track_offset) c0, c1 -> (Telescope::calc_offload) -> sht0, sht1 -> (Derived::update_telescope_track) -> ctrl0, ctrl1
 
 \section telescope_cfg Configuration params
 
 - scalefac <s0> <s1>: Telescope::scalefac
 - rotation <ang>: Telescope::ccd_ang
 - ttgain <gain>: Telescope::ttgain
 - cadence <delay>: Telescope::handler_p
 
 \section telescope_netio Network commands
 
 - set scalefac <s0> <s1>: Telescope::scalefac
 - get scalefac: return Telescope::scalefac as <s0> <s1>
 - set ttgain <p> <i> <d>: Telescope::ttgain
 - get ttgain: return Telescope::ttgain as <p> <i> <d>
 - set ccd_ang <ang>: Telescope::ccd_ang
 - get ccd_ang <ang>: Telescope::ccd_ang
 - get shifts: return Telescope::c0 Telescope::c1 Telescope::sht0 Telescope::sht1 Telescope::ctrl0 Telescope::ctrl1
 - get pix2shiftstr: return coordinate conversion formula.
 - get tel_track: return Telescope::telpos
 - get tel_units: return Telescope::tel_units
 
*/
class Telescope: public foam::Device {
protected:
	pthread::thread tel_thr;			//!< Telescope handler thread
	void tel_handler();						//!< Handler thread that takes care of converting input shifts to tracking the telescope correctly

	float c0;								//!< Input offset (arbitary units, most likely pixels)
	float c1;								//!< Input offset (arbitary units, most likely pixels)	

	float sht0;							//!< Output shift, scaled and rotated
	float sht1;							//!< Output shift, scaled and rotated
	
	float ctrl0;						//!< Final control coordinates for telescope
	float ctrl1;						//!< Final control coordinates for telescope

public:
	Telescope(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online=true);
	~Telescope();
	
	double telpos[2];				//!< Telescope position (e.g. alt/az)
	string telunits[2];			//!< Telescope units
	float scalefac[2];			//!< Scale factor for setup that converts input units to 1mm shift in primary focus
	gain_t ttgain;					//!< Gain for tip-tilt offloading correction
	float ccd_ang;					//!< Rotation of CCD with respect to telescope restframe
	
	float handler_p;				//!< Handler update period (seconds)
	bool do_offload;				//!< Enable or disable live offloading
	
	void set_track_offset(const float _c0, const float _c1) { c0 = _c0; c1 = _c1; }
	void get_track_offset(float * const _c0, float * const _c1) { *_c0 = c0; *_c1 = c1; }
	
	/*! @brief Convert shift vector from instrument pixels to generic coordinates
	 
	 Convert from instrument pixels to millimeters or something similar. This is 
	 done with shift_vec = rot_mat(ccd_ang) # (scalefac * input_vec).
	 
	 Output is stored in Telescope::sht0, Telescope::sht1.
	 
	 @param [in] insh0 Shift vector dimension 0
	 @param [in] insh1 Shift vector dimension 1
	 */
	void calc_offload(const float insh0, const float insh1);
	
	/*! @brief Given generic shift coordinates, track the telescope
	 
	 This function needs to be implemented in specific telescope classes because
	 we do not know here what coordinat system we should use. If not it will do 
	 random stuff.
	 
	 @param [in] sht0 Generic shift dimension 0
	 @param [in] sht1 Generic shift dimension 1
	 */
	virtual int update_telescope_track(const float sht0, const float sht1) { c0 = (c0 + 0.1); c1 = (c1 + 0.5); return 0; }

	// From Devices::
	virtual void on_message(Connection *const conn, string);
};

#endif // HAVE_TELESCOPE_H

/*!
 \page dev_telescope Telescope tracking class
 
 Most high-speed wavefront corretors (DMs) can only correct so much tip-tilt. 
 Over longer observations, this has to be offloaded to a larger-stroke device,
 generally a telescope. This is done here.

 \section dev_telescope_der Derived classes
 - \subpage dev_telescope_wht "William Herschel Telescope guiding control"
*/

