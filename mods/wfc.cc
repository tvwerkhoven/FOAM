/*
 wfc.cc -- a wavefront corrector base class
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <string>
#include <stdint.h>
#include <sys/types.h>

#include "types.h"
#include "config.h"
#include "path++.h"

#include "foamctrl.h"
#include "devices.h"
#include "wfc.h"

Wfc::Wfc(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const & conffile, const bool online):
Device(io, ptc, name, wfc_type + "." + type, port, conffile, online),
wfc_amp(NULL), nact(0), gain(0.0, 0.0, 0.0) {	
	io.msg(IO_DEB2, "Wfc::Wfc()");
	
	add_cmd("set gain");
	add_cmd("get gain");
}

Wfc::~Wfc() {
	io.msg(IO_DEB2, "Wfc::~Wfc()");
}

int Wfc::calibrate() {
	// Allocate memory for control command
	if (wfc_amp)
		gsl_vector_float_free(wfc_amp);
		
	wfc_amp = gsl_vector_float_calloc(nact);

	set_calib(true);
	return 0;
}

void Wfc::on_message(Connection *const conn, string line) { 
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {							// get ...
		string what = popword(line);
		
		if (what == "gain") {							// get gain
			conn->addtag("gain");
			conn->write(format("ok gain %g %g %g", gain.p, gain.i, gain.d));
		} else
			parsed = false;

	} else if (command == "set") {			// set ...
		string what = popword(line);
		
		if (what == "gain") {							// set gain <p> <i> <d>
			conn->addtag("gain");
			gain.p = popdouble(line);
			gain.i = popdouble(line);
			gain.d = popdouble(line);
			netio.broadcast(format("ok gain %g %g %g", gain.p, gain.i, gain.d));
		} else
			parsed = false;
	}
		
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}
