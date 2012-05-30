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
Device(io, ptc, name, telescope_type + "." + type, port, conffile, online)
{
	io.msg(IO_DEB2, "Telescope::Telescope()");
	
	// Configure initial settings
	{
		scalefac[0] = cfg.getdouble("scalefac_0", 1e-2);
		scalefac[1] = cfg.getdouble("scalefac_1", 1e-2);
		gain.p = cfg.getdouble("gain_p", 1.0);
		ccd_ang = cfg.getdouble("ccd_ang", 0.0);
		handler_p = cfg.getdouble("cadence", 1.0);
	}

	add_cmd("get scalefac");
	add_cmd("set scalefac");
	add_cmd("get gain");
	add_cmd("set gain");
	add_cmd("get ccd_ang");
	add_cmd("set ccd_ang");

	// Start handler thread
	tel_thr.create(sigc::mem_fun(*this, &Telescope::tel_handler));
}

void Telescope::tel_handler() {
	struct timeval last, now, diff;
	float this_sleep;
	
	while (ptc->mode != AO_MODE_SHUTDOWN) {
		io.msg(IO_DEB1, "Telescope::tel_handler() looping...");
		gettimeofday(&last, 0);
		
		// From input offset (in arbitrary units, i.e. pixels), calculate proper 
		// shift coordinates, i.e.: 
		// shift_vec = rot_mat * scale_vec * input_vec
		// General:
		// x' = scalefac0 [ x cos(th) - y sin(th) ]
		// y; = scalefac1 [ x sin(th) + y cos(th) ]
		sht0 = scalefac[0] * c0 * cos(ccd_ang * 180.0/M_PI) - scalefac[1] * c1 * sin(ccd_ang * 180.0/M_PI);
		sht1 = scalefac[0] * c0 * sin(ccd_ang * 180.0/M_PI) + scalefac[1] * c1 * cos(ccd_ang * 180.0/M_PI);
		
		io.msg(IO_DEB1, "Telescope::tel_handler() (%g, %g) -> (%g, %g)", c0, c1, sht0, sht1);
		update_telescope_track(sht0, sht1);

		// Make sure each iteration takes at minimum handler_p seconds:
		// update duration = now - last
		// loop period should be handler_p
		// sleep time = handler_p - (now - last)
		gettimeofday(&now, 0);
		timerclear(&diff);
		timersub(&now, &last, &diff);
		
		this_sleep = handler_p * 1E6 - (diff.tv_sec * 1E6 + diff.tv_usec);
		if(this_sleep > 0)
			usleep(this_sleep);
	}
}

Telescope::~Telescope() {
	io.msg(IO_DEB2, "Telescope::~Telescope()");

	//!< @todo Save all device settings back to cfg file
	cfg.set("scalefac", scalefac);
	
	// Join with telescope handler thread
	tel_thr.cancel();
	tel_thr.join();
}

void Telescope::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {
		string what = popword(line);
		
		if (what == "scalefac") {					// get scalefac
			conn->addtag("scalefac");
			conn->write(format("ok scalefac %g %g", scalefac[0], scalefac[1]));
		} else if (what == "gain") {			// get gain
			conn->addtag("gain");
  		conn->write(format("ok gain %g %g %g", gain.p, gain.i, gain.d));
		} else if (what == "ccd_ang") {		// get ccd_ang - CCD rotation angle
			conn->addtag("ccd_ang");
			conn->write(format("ok ccd_ang %g", ccd_ang));
		} else 
			parsed = false;
	} else if (command == "set") {
		string what = popword(line);
		
		if (what == "scalefac") {					// set scalefac <s0> <s1>
			conn->addtag("scalefac");
			scalefac[0] = popdouble(line);
			scalefac[1] = popdouble(line);
		} else if (what == "gain") {			// set gain <p> <i> <d>
			conn->addtag("gain");
			gain.p = popdouble(line);
			gain.i = popdouble(line);
			gain.d = popdouble(line);
		} else if (what == "ccd_ang") {		// set ccd_ang <ang>
			conn->addtag("ccd_ang");
			ccd_ang = popdouble(line);
		} else
			parsed = false;

		parsed = false;
	} else {
		parsed = false;
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}