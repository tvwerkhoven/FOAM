/*
 simseeing.h -- atmosphere/telescope simulator header
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_SIMSEEING_H
#define HAVE_SIMSEEING_H

#include <stdint.h>
#include <gsl/gsl_matrix.h>

#include "types.h"
#include "config.h"
#include "io.h"
#include "devices.h"

using namespace std;
const string SimSeeing_type = "simseeing";

/*!
 @brief This class simulates seeing by an atmosphere.
 */
class SimSeeing: public Device {
private:
	
	struct wavefront_t {
		gsl_matrix *data;					//!< Holds the wavefront data
		
		string type;							//!< Auto-generate wavefront or load from disk ('auto' or 'file')
		Path file;								//!< If type = 'file', this is the full path
		double r0;								//!< If type = 'auto', this is Fried's r_0 parameter [cm]

		fcoord_t windspeed;				//!< Windspeed in pixels/frame
	} wf;
	
	gsl_matrix *wf_crop;				//!< This will hold the cropped wavefront data
	coord_t crop_pos;						//!< Lower-left position to crop out of wavefront
	
	gsl_matrix *load_wavefront(); //!< Load wavefront data from disc
	gsl_matrix *gen_wavefront(); //!< Generate wavefront data from scratch
	
public:
	SimSeeing(Io &io, foamctrl *ptc, string name, string port, Path &conffile);
	~SimSeeing();
	
	gsl_matrix *get_wavefront();
	gsl_matrix *get_wavefront(int x0, int y0, int x1, int y1);	
};

#endif // HAVE_SIMSEEING_H
