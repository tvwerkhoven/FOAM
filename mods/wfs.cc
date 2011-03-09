/*
 wfs.cc -- a wavefront sensor abstraction class
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
#include <gsl/gsl_vector.h>

#include "types.h"
#include "io.h"

#include "zernike.h"
#include "wfs.h"

using namespace std;

// Constructor / destructors

Wfs::Wfs(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, Camera &wfscam, const bool online):
Device(io, ptc, name, wfs_type, port, conffile, online),
zernbasis(io, 0, wfscam.get_width()),
cam(wfscam)
{	
	io.msg(IO_DEB2, "Wfs::Wfs()");
	init();
}

Wfs::Wfs(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, Camera &wfscam, const bool online):
Device(io, ptc, name, wfs_type + "." + type, port, conffile, online),
zernbasis(io, 0, wfscam.get_width()),
cam(wfscam)
{	
	io.msg(IO_DEB2, "Wfs::Wfs()");
	init();
}

void Wfs::init() {
	add_cmd("measuretest");
	add_cmd("get modes");
	add_cmd("get basis");
	add_cmd("get calib");
	add_cmd("get camera");
}

Wfs::~Wfs() {
	io.msg(IO_DEB2, "Wfs::~Wfs()");
}


void Wfs::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "measuretest") {
		// Specifically call Wfs::measure() to fake a measurement
		Wfs::measure();
		conn->addtag("measuretest");
		conn->write("ok measuretest");
	} else if (command == "get") {
		string what = popword(line);
		
		if (what == "modes") {
			conn->addtag("modes");
			string moderep = "";
			for (int n=0; n<wf.nmodes; n++) {
				moderep += format("%4f ", gsl_vector_float_get(wf.wfamp, n));
			}
			conn->write(format("ok modes %d %s", wf.nmodes, moderep.c_str()));
		} else if (what == "camera") {
			conn->addtag("camera");
			conn->write("ok camera " + cam.name);
		} else if (what == "calib") {
			conn->addtag("calib");
			conn->write(format("ok calib %d", is_calib));
		} else if (what == "basis") {
			conn->addtag("basis");
			string tmp;
			if (wf.basis == ZERNIKE) tmp = "zernike";
			else if (wf.basis == KL) tmp = "kl";
			else if (wf.basis == MIRROR) tmp = "mirror";
			else if (wf.basis == SENSOR) tmp = "sensor";
			else tmp = "unknown";
			conn->write(format("ok basis %s", tmp.c_str()));
 		}
		else
			parsed = false;
	}
	else
		parsed = false;
	

	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}

int Wfs::measure() {
	io.msg(IO_DEB2, "Wfs::measure(), filling random");
	if (wf.nmodes == 0) {
		wf.nmodes = 16;
		wf.wfamp = gsl_vector_float_alloc(wf.nmodes);
		wf.basis = SENSOR;
	}
	
	for (int n=0; n<wf.nmodes; n++) {
		gsl_vector_float_set(wf.wfamp, n, drand48()*2.0-1.0);
	}
	
	return 0;
}

int Wfs::calibrate() {
	io.msg(IO_DEB2, "Wfs::calibrate()");
	is_calib = true;

	return 0;
}

