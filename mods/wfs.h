/*
 wfs.h -- a wavefront sensor abstraction class
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

#ifndef HAVE_WFS_H
#define HAVE_WFS_H

#include <string>
#include <gsl/gsl_vector.h>

#include "types.h"
#include "config.h"
#include "camera.h"
#include "io.h"

using namespace std;

static const string wfs_type = "wfs";

/*!
 @brief Base wavefront-sensor class. 
 
 This class provides a template for the implementation of wavefront sensors 
 such as a Shack-Hartmann WFS. This class should be implemented with WFS
 specific routines. The Wfs class is independent of the camera used and
 only provides the data processing/interpretation of the camera. The camera
 itself can be accessed through the reference &cam.
 */
class Wfs: public Device {
public:	
	enum wfbasis {
		ZERNIKE=0,												//!< Zernike
		KL,																//!< Karhunen-Loéve 
		MIRROR,														//!< Mirror modes
		SENSOR,														//!< Sensor modes (e.g. shift vectors)
		UNDEFINED
	};
	
	/*!
	 @brief This holds information on the wavefront
	 */
	struct wavefront {
		wavefront() : wfamp(NULL), nmodes(0), basis(SENSOR) { ; }
		gsl_vector_float *wfamp;					//!< Mode amplitudes
		int nmodes;												//!< Number of modes
		enum wfbasis basis;								//!< Basis functions used for this representation
	};
	
	struct wavefront wf;								//!< Wavefront representation
	bool is_calib;											//!< Is calibrated & ready for use
	
	Camera &cam;												//!< Reference to the camera class used for this WFS
	
	// To be implemented:
	virtual int measure();							//!< Measure abberations
	virtual int calibrate();						//!< Calibrate sensor, set up reference and mode basis
	
	// From Device::
	virtual void on_message(Connection *const conn, string line);
	
	virtual ~Wfs();
	Wfs(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, Camera &wfscam, const bool online=true); //!< Constructor for derived WFSs (i.e. SHWFS)
	Wfs(Io &io, foamctrl *const ptc, const string name, const string port, Path const & conffile, Camera &wfscam, const bool online=true); //!< Constructor for bare WFS
};

#endif // HAVE_WFS_H

/*!
 \page dev_wfs Wavefront sensor devices
 
 The Wfs class provides control for wavefront sensors.
 
 \section moreinfo More information
 - \subpage dev_wfs_shwfs "Shack-Hartmann wavefront sensor device"

 \section related See also
 - \ref dev_cam "Camera devices"
 
*/ 
