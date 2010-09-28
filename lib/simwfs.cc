/*
 simwfs.cc -- simulate a wavefront sensor
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


SimWfs::SimWfs(Io &io, foamctrl *ptc, string name, string type, string port, string conffile):
Device(io, ptc, name, dev_type + "." + type, port, conffile)
{
	io.msg(IO_DEB2, "SimWfs::SimWfs()");
	//! @todo init wavefront sensor, read config, check stuff
}

SimWfs::~SimWfs() {
	io.msg(IO_DEB2, "SimWfs::~SimWfs()");
}

gsl_matrix *SimWfs::sim_shwfs(gsl_matrix *wavefront) {
	io.msg(IO_DEB2, "SimWfs::sim_shwfs()");
	//! @todo Given a wavefront, image it through a system and return the resulting intensity pattern (i.e. an image).
}
