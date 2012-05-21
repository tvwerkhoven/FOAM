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
		scalefac[0] = cfg.getdouble("scalefac_0", 1e-2);
		scalefac[1] = cfg.getdouble("scalefac_1", 1e-2);
		gain.p = cfg.getdouble("gain_p", 1.0);
		ccd_ang = cfg.getdouble("ccd_ang", 0.0);
	}
}

Telescope::~Telescope() {
	io.msg(IO_DEB2, "Telescope::~Telescope()");

	//!< @todo Save all device settings back to cfg file
	cfg.set("scalefac", scalefac);
}

void Telescope::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {
		string what = popword(line);
		
		if (what == "scalefac") {					// get shifts
			conn->addtag("gain");
			conn->write(format("ok scalefac %g %g", scalefac[0], scalefac[1]));
		} else if (what == "gain") {				// get gain
			conn->addtag("gain");
  		conn->write(format("ok gain %g %g %g", gain.p, gain.i, gain.d));
		} else if (what == "rotang") {					// get rot - rotation angle
			conn->addtag("rotation");
			conn->write(format("ok rotation %g", ccd_ang));
		} else 
			parsed = false;
	} else if (command == "set") {
		string what = popword(line);
		
		parsed = false;
	} else {
		parsed = false;
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}
