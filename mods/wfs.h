/*
 wfs.h -- a wavefront sensor abstraction class
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
 @file wfs.h
 @brief Base wavefront sensor class
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 */

#ifndef HAVE_WFS_H
#define HAVE_WFS_H

#include <string>
#include <gsl/gsl_vector.h>

#include "types.h"
#include "config.h"
#include "camera.h"
#include "io.h"

static const string wfs_type = "wfs";

/*!
 @brief Base wavefront-sensor class. 
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This class provides a template for the implementation of wavefront sensors 
 such as a Shack-Hartmann WFS. This class should be implemented with WFS
 specific routines. The Wfs class is independent of the camera used and
 only provides the data processing/interpretation of the camera. The camera
 itself can be accessed through the pointer *cam.
 */
class Wfs: public Device {
public:	
	
	/*!
	 @brief This holds information on the wavefront
	 */
	struct wavefront {
		wavefront() : wfamp(NULL), nmodes(0) { ; }
		gsl_vector_float *wfamp;					//!< Mode amplitudes
		int nmodes;												//!< Number of modes
		enum {
			ZERNIKE=0,											//!< Zernike modes
			KL,															//!< Karhunen-Loeve modes
		} wfmode;
	};
	
	Camera *cam;												//!< Pointer to the camera class used for this 
		
	virtual int verify(int) = 0;				//!< Verify settings
	virtual int calibrate(int) = 0;			//!< Calibrate WFS
	virtual int measure(int) = 0;				//!< Measure abberations
	
	virtual ~Wfs() {}
	Wfs(Io &io, foamctrl *ptc, string name, string type, string port, Path &conffile):
	Device(io, ptc, name, cam_type + "." + type, port, conffile) {	; }
};

#endif // HAVE_WFS_H
