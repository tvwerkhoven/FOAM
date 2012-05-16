/*
 telescope.cc -- offload tip-tilt correction to some large-stroke device (telescope)
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

#include "devices.h"

#include "telescope.h"

using namespace std;

Telescope::Telescope(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online):
Device(io, ptc, name, telescope_type + "." + type, port, conffile, online),
{
	io.msg(IO_DEB2, "Telescope::Telescope()");
	
	// Configure initial settings
	{
		scalefac = cfg.getdouble("scalefac", 1e-2);
	}
}

Telescope::~Telescope() {
	io.msg(IO_DEB2, "Telescope::~Telescope()");

	//!< @todo Save all device settings back to cfg file
	cfg.set("scalefac", scalefac);
}
