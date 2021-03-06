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
c0(0), c1(0), sht0(0), sht1(0), ctrl0(0), ctrl1(0), ccd_ang(0), 
handler_p(0), do_offload(true)
{
	io.msg(IO_DEB2, "Telescope::Telescope()");
	
	// Init static vars
	telpos[0] = telpos[1] = 0;
	telunits[0] = "ax0";
	telunits[1] = "ax1";

	ttgain.p = ttgain.i = ttgain.d = 0.0;

	// Configure initial settings
	{
		scalefac[0] = cfg.getdouble("scalefac_0", 1e-2);
		scalefac[1] = cfg.getdouble("scalefac_1", 1e-2);
		ttgain.p = cfg.getdouble("ttgain_p", 1.0);
		ccd_ang = cfg.getdouble("ccd_ang", 0.0);
		handler_p = cfg.getdouble("cadence", 1.0);
	}

	add_cmd("get scalefac");
	add_cmd("set scalefac");
	add_cmd("get ttgain");
	add_cmd("set ttgain");
	add_cmd("get ccd_ang");
	add_cmd("set ccd_ang");
	add_cmd("get shifts");
	add_cmd("get pix2shiftstr");
	add_cmd("get tel_track");
	add_cmd("get tel_units");
	add_cmd("get offloading");
	add_cmd("set offloading");
	add_cmd("track pixshift");
	add_cmd("track telshift");

	// Start handler thread
	tel_thr.create(sigc::mem_fun(*this, &Telescope::tel_handler));
}

void Telescope::calc_offload(const float insh0, const float insh1) {
	// From input offset (in arbitrary units, i.e. pixels in the WFS camera), 
	// calculate proper shift coordinates, i.e.: 
	// shift_vec = rot_mat # (scale_vec * input_vec)
	// General:
	// x' = scalefac0 [ x cos(th), - y sin(th) ]
	// y; = scalefac1 [ x sin(th), + y cos(th) ]
	sht0 = scalefac[0] * insh0 * cos(ccd_ang * M_PI/180.0) - scalefac[1] * insh1 * sin(ccd_ang * M_PI/180.0);
	sht1 = scalefac[0] * insh0 * sin(ccd_ang * M_PI/180.0) + scalefac[1] * insh1 * cos(ccd_ang * M_PI/180.0);
	
	// Then feed these coordinates to the telescope which can convert it to 
	//telescope-specific commands
	io.msg(IO_DEB1, "Telescope::calc_offload(%f, %f) -> %f, %f", insh0, insh1, sht0, sht1);
	update_telescope_track(sht0, sht1);
}

void Telescope::tel_handler() {
	struct timeval last, now, diff;
	float this_sleep;
	
	while (ptc->mode != AO_MODE_SHUTDOWN) {
		gettimeofday(&last, 0);
		
		// Calculate offload and track the telescope to the correct position
		if (do_offload)
			calc_offload(c0, c1);

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

	//! @todo Save all device settings back to cfg file
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
		} else if (what == "ttgain") {			// get ttgain
			conn->addtag("ttgain");
  		conn->write(format("ok ttgain %g", ttgain.p));
		} else if (what == "tel_track") {	// get tel_track - Telescope tracking position
			conn->write(format("ok tel_track %g %g", telpos[0], telpos[1]));
		} else if (what == "tel_units") {	// get tel_units - Telescope tracking units
			conn->write(format("ok tel_units %s %s", telunits[0].c_str(), telunits[1].c_str()));
		} else if (what == "offloading") {		// get offloading
			conn->write(format("ok offloading %d", (int) do_offload));
		} else if (what == "ccd_ang") {		// get ccd_ang - CCD rotation angle
			conn->addtag("ccd_ang");
			conn->write(format("ok ccd_ang %g", ccd_ang));
		} else if (what == "shifts") {		// get shifts - Give raw shifts, conv shifts, ctrl shifts
			conn->addtag("shifts");
			conn->write(format("ok shifts %g %g %g %g %g %g", c0, c1, sht0, sht1, ctrl0, ctrl1));
		} else if (what == "pix2shiftstr") {		// get pix2shiftstr - Give conversion formula as string
			conn->addtag("pix2shiftstr");
			conn->write("ok pix2shiftstr scalefac[0] * c0 * <cos,sin>(ccd_ang * 180.0/M_PI) + scalefac[1] * c1 * <-sin,cos>(ccd_ang * 180.0/M_PI)");
		} else 
			parsed = false;
	} else if (command == "set") {
		string what = popword(line);
		
		if (what == "scalefac") {					// set scalefac <s0> <s1>
			conn->addtag("scalefac");
			scalefac[0] = popdouble(line);
			scalefac[1] = popdouble(line);
			conn->addtag("scalefac");
			conn->write(format("ok scalefac %g %g", scalefac[0], scalefac[1]));
		} else if (what == "ttgain") {			// set ttgain <p> <i> <d>
			conn->addtag("ttgain");
			ttgain.p = popdouble(line);
			conn->addtag("ttgain");
  		conn->write(format("ok ttgain %g", ttgain.p));
		} else if (what == "offloading") {		// set offloading <0,1>
			do_offload = popint(line);
			conn->write(format("ok offloading %d", (int) do_offload));
		} else if (what == "ccd_ang") {		// set ccd_ang <ang>
			conn->addtag("ccd_ang");
			ccd_ang = popdouble(line);
			conn->addtag("ccd_ang");
			conn->write(format("ok ccd_ang %g", ccd_ang));
		} else
			parsed = false;
	} else if (command == "track") {
		string what = popword(line);
		
		if (what == "pixshift") {						// track pixshift <pixel0> <pixel1>
			c0 = popdouble(line);
			c1 = popdouble(line);
			calc_offload(c0, c1);
			conn->write(format("ok track pixshift %f %f", c0, c1));
		} else if (what == "telshift") {		// track telshift <shift0> <shift1>
			sht0 = popdouble(line);
			sht1 = popdouble(line);
			update_telescope_track(sht0, sht1);
			conn->write(format("ok track telshift %f %f", sht0, sht1));
		} else
			parsed = false;
	} else {
		parsed = false;
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}
