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
	gsl_matrix *wfsrc;					//!< Holds the wavefront data (can be bigger than wfsize)
	Path file;									//!< If type = 'file', this is the full path
	
	gsl_matrix *wfcrop;					//!< This will hold the cropped wavefront data
	coord_t croppos;						//!< Lower-left position to crop out of wavefront
	
	coord_t cropsize;						//!< Size of the wavefront to return
	coord_t windspeed;					//!< Windspeed in pixels/frame

	gsl_matrix *load_wavefront(); //!< Load wavefront data from disc
	
public:
	SimSeeing(Io &io, foamctrl *ptc, string name, string port, Path &conffile);
	~SimSeeing();
	
	bool setup(Path &f, coord_t size, coord_t wspeed);
	gsl_matrix_view get_wavefront();
	gsl_matrix_view get_wavefront(size_t x0, size_t y0, size_t w, size_t h);
};

#endif // HAVE_SIMSEEING_H
