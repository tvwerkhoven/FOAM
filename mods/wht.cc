/*
 wht.h -- William Herschel Telescope control
 Copyright (C) 2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#include <math.h>
#include <string>
#include <gsl/gsl_blas.h>

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include "utils.h"
#include "io.h"
#include "config.h"
#include "csv.h"
#include "pthread++.h"

#include "telescope.h"
#include "devices.h"

using namespace std;

WHT::WHT(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online):
Telescope(io, ptc, name, telescope_type + "." + type, port, conffile, online),
{
	io.msg(IO_DEB2, "WHT::WHT()");
	
	// Configure initial settings
	{
		// port
		// parity
		// speed
		// delimiter
		// coords url
	}
}

WHT::~WHT() {
	io.msg(IO_DEB2, "WHT::~WHT()");

	//!< @todo Save all device settings back to cfg file
}

int WHT::get_wht_coords(float *c0, float *c1) {
	// Open URL
	
	// Read data
	
	// Parse data
	
	// Set c0, c1
	
	return 0;
}
