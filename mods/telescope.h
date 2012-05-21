/*
 telescope.h -- offload tip-tilt correction to some large-stroke device (telescope)
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
 
 - Should be WFC or just Device? -> Device
 - Need children that does hardware interface, and conversion form unit pointing to image shift
 - Need configuration that tells magnification from primary focus to camera
 
 \section telescope_cfg Configuration params
 
 - scalefac <s0> <s1>: Telescope::scalefac
 - rotation <ang>: Telescope::ccd_ang
 
 \section telescope_netio Network commands
 
 - set scalefac <s0> <s1>: Telescope::scalefac
 - get scalefac: return Telescope::scalefac as <s0> <s1>
 - set gain <p> <i> <d>: Telescope::gain
 - get gain: return Telescope::gain as <p> <i> <d>
 
*/
class Telescope: public foam::Device {
private:
	
public:
	Telescope(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online=true);
	~Telescope();
	
	float scalefac[2];			//!< Scale factor for setup that converts input units to 1mm shift in primary focus
	gain_t gain;						//!< Gain for tip-tilt offloading correction
	float ccd_ang;					//!< Rotation of CCD with respect to telescope restframe
	
//	virtual int delta_pointing(const float dc0, const float dc1);
//	virtual int conv_pix_point(const float dpix0, const float dpix1, float *dc0, float *dc1);

	// From Devices::
	void on_message(Connection *const conn, string);
};

#endif // HAVE_TELESCOPE_H

/*!
 \page dev_telescope Telescope tracking class
 
 Most high-speed wavefront corretors (DMs) can only correct so much tip-tilt. 
 Over longer observations, this has to be offloaded to a larger-stroke device,
 generally a telescope. This is done here.
 */

