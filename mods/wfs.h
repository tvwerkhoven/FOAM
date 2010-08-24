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
#include "cam.h"
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
class Wfs {
protected:
	string conffile;
	Io &io;
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	string name;												//!< WFS name
	string camtype;											//!< Camera type/model
	string wfstype;											//!< WFS type/model
	config cfg;													//!< WFS configuration class
	
	/*!
	 @brief This holds information on the wavefront
	 */
	struct wavefront {
		gsl_vector_float *wfamp;					//!< Mode amplitudes
		enum {
			MODES_ZERNIKE=0,								//!< Zernike modes
			MODES_KL,												//!< Karhunen-Loeve modes
		} wfmode;
		int nmodes;												//!< Number of modes
	};
	
	Camera *cam;												//!< Pointer to the camera class used for this 
		
	typedef struct {
		int prop;
		void *value;
	} wfs_prop_t;												//!< WFS property structure
	
	virtual int verify(int) = 0;				//!< Verify settings
	virtual int calibrate(int) = 0;			//!< Calibrate WFS
	virtual int measure(int) = 0;				//!< Measure abberations
	virtual int configure(wfs_prop_t *) = 0; //!< Change a WFS setting
	
	virtual ~Wfs() {}
	Wfs(Io &io, string conffile): io(io), cfg(conffile) {
		io.msg(IO_DEB2, "Wfs(Io &io, string conffile=%s)", conffile.c_str());
	}
};

#endif // HAVE_WFS_H
