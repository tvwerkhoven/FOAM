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

#include "foamctrl.h"
#include "io.h"
#include "simseeing.h"

SimSeeing::SimSeeing(Io &io, foamctrl *ptc, string name, string type, string port, string conffile):
Device(io, ptc, name, simseeing_type + "." + type, port, conffile)
{
	io.msg(IO_DEB2, "SimSeeing::SimSeeing()");
	//! @todo init wavefront here, matrices, read config etc
}

SimSeeing::~SimSeeing() {
	io.msg(IO_DEB2, "SimSeeing::~SimSeeing()");
}

gsl_matrix *SimSeeing::get_wavefront() {
	io.msg(IO_DEB2, "SimSeeing::get_wavefront()");
	//! @todo look at windspeed etc, get new wavefront using get_wavefront(...)
}

gsl_matrix *SimSeeing::get_wavefront(int x0, int y0, int x1, int y1) {
	io.msg(IO_DEB2, "SimSeeing::get_wavefront(...)");
	//! @todo get wavefront crop
}

