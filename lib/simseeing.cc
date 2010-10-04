/*
 simseeing.cc -- atmosphere/telescope simulator
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

#include <string>
#include <gsl/gsl_matrix.h>

#include "imgdata.h"

#include "foamctrl.h"
#include "imgio.h"
#include "io.h"
#include "simseeing.h"

/*
 *  Constructors / destructors
 */

SimSeeing::SimSeeing(Io &io, foamctrl *ptc, string name, string port, Path &conffile):
Device(io, ptc, name, SimSeeing_type, port, conffile)
{
	io.msg(IO_DEB2, "SimSeeing::SimSeeing()");
	//! @todo init wavefront here, matrices, read config etc
	
	wf.type = cfg.getstring("type");
	if (wf.type != "file" && wf.type != "auto")
		throw exception("SimSeeing::SimSeeing() type should be either 'auto' or 'file', not '" + wf.type + "'.");
	
	if (wf.type == "auto") {
		io.msg(IO_INFO, "SimSeeing::SimSeeing() Auto-generating wavefront.");
		wf.r0 = cfg.getdouble("wf_r0", 10.0);
		wf.data = gen_wavefront();
	}
	else {
		wf.file = ptc->confdir + wf.file;
		wf.data = load_wavefront();
	}

	io.msg(IO_DEB2, "SimSeeing::SimSeeing() init done");
}

SimSeeing::~SimSeeing() {
	io.msg(IO_DEB2, "SimSeeing::~SimSeeing()");
}

/*
 *  Private methods
 */

gsl_matrix *SimSeeing::gen_wavefront() {
	io.msg(IO_DEB2, "SimSeeing::gen_wavefront()");
	
	return NULL;
}

gsl_matrix *SimSeeing::load_wavefront() {
	io.msg(IO_DEB2, "SimSeeing::load_wavefront()");
	
	if (!wf.file.r())
		throw exception("SimSeeing::load_wavefront() cannot read wavefront file: " + wf.file.str() + "!");
	
	ImgData wftmp(io, wf.file, ImgData::AUTO);
	if (wftmp.err) {
		io.msg(IO_ERR, "SimSeeing::load_wavefront() ImgData returned an error: %d\n", wftmp.err);
		return NULL;
	}
	
	return wftmp.as_GSL(true);
}

/*
 *  Public methods
 */

gsl_matrix *SimSeeing::get_wavefront() {
	io.msg(IO_DEB2, "SimSeeing::get_wavefront()");
	//! @todo look at windspeed etc, get new wavefront using get_wavefront(...)
	return NULL;
}

gsl_matrix *SimSeeing::get_wavefront(int x0, int y0, int x1, int y1) {
	io.msg(IO_DEB2, "SimSeeing::get_wavefront(...)");
	//! @todo get wavefront crop
	return NULL;
}

