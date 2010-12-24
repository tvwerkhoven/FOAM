/*
 wfs.cc -- a wavefront sensor abstraction class
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
 @file wfs.cc
 @brief Base wavefront sensor class
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 */

#include <string>
#include <gsl/gsl_vector.h>

#include "types.h"
#include "io.h"
#include "wfs.h"

Wfs::Wfs(Io &io, foamctrl *ptc, string name, string type, string port, Path conffile, Camera &wfscam, bool online):
Device(io, ptc, name, wfs_type + "." + type, port, conffile, online),
cam(wfscam)
{	
	io.msg(IO_DEB2, "Wfs::Wfs()");
	
	add_cmd("measuretest");
	add_cmd("get modes");
	
}

Wfs::~Wfs() {
		io.msg(IO_DEB2, "Wfs::~Wfs()");
}


void Wfs::on_message(Connection *conn, std::string line) {
	io.msg(IO_DEB2, "Wfs::on_message()");
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "measuretest") {
		measure();
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
		}
		else
			parsed = false;
		//! @todo basis
//		else if (what == "basis") {
//			conn->addtag("basis");
//			conn->write(format("ok basis %lf", noiseamp));
// 		}
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
		wf.basis = MIRROR;
	}
	
	for (int n=0; n<wf.nmodes; n++) {
		gsl_vector_float_set(wf.wfamp, n, drand48()*2.0-1.0);
	}
	
	return 0;
}

