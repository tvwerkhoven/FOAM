/*
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */
/*! 
	@file whwfs.ccc
	@author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)

	@brief This file contains modules and functions related to Shack-Hartmann 
	wavefront sensing.
*/

// HEADERS //
/***********/

#include "config.h"
#include "types.h"
#include "cam.h"
#include "io.h"
#include "wfs.h"

#define SHWFS_TYPE "sh"
extern Io *io;

// ROUTINES //
/************/

class Shwfs: public Wfs {
	coord_t cells;
	
	public:
	Shwfs(config &config) {
		io->msg(IO_DEB2, "Shwfs::Shwfs(config &config)");
		
		conffile = config.filename;
		
		wfstype = config.getstring("type");
		if (wfstype != SHWFS_TYPE) throw exception("Type should be 'sh' for this class.");
		name = config.getstring("name");
		cells.x = config.getint("cellsx");
		cells.y = config.getint("cellsy");
				
		io->msg(IO_XNFO, "Shwfs init complete got %dx%d cells.", cells.x, cells.y);
		
		int idx = conffile.find_last_of("/");
		string camcfg = config.getstring("camcfg");
		if (camcfg[0] != '/') camcfg = conffile.substr(0, idx) + "/" + camcfg;
		cam = Camera::create(camcfg);
	}
	
	~Shwfs() {
		io->msg(IO_DEB2, "Shwfs::~Shwfs()");
		if (cam) delete cam;
	}
};

Wfs *Wfs::create(config &config) {
	io->msg(IO_DEB2, "Wfs::create(config &config)");
	return new Shwfs(config);
}
