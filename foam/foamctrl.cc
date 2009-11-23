/*
 foamctrl.cc -- control class for complete AO system
 
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

#include <time.h>

#include "foamctrl.h"
#include "types.h"
#include "config.h"
#include "io.h"

extern Io *io;

foamctrl::~foamctrl(void) {
	io->msg(IO_DEB2, "foamctrl::~foamctrl(void)");
	delete cfgfile;
	delete[] wfs;
	delete[] wfc;
	delete[] filter;
}

foamctrl::foamctrl(void) { 
	io->msg(IO_DEB2, "foamctrl::foamctrl(void)");
	starttime = time(NULL); 
	frames = 0; 
	wfs_count = wfc_count = fw_count = 0;
}

foamctrl::foamctrl(string &file) {
	io->msg(IO_DEB2, "foamctrl::foamctrl(string &file)");
	starttime = time(NULL); 
	frames = 0; 
	wfs_count = wfc_count = fw_count = 0;
	parse(file);
}

int foamctrl::parse(string &file) {
	io->msg(IO_DEB2, "foamctrl::parse(string &file)");
	conffile = file;
	cfgfile = new config(conffile);
	
	// Configure AO system now
	try {
		mode = AO_MODE_LISTEN;		// start in listen mode
		calmode = CAL_INFL;
		
		wfs_count = cfgfile->getint("wfs_count");
		wfc_count = cfgfile->getint("wfc_count");
		fw_count = 0;//cfgfile->getint("fw_count");
		
		// Allocate memory
		wfs = new Wfs[wfs_count];
		wfc = new Wfc[wfc_count];
		filter = new fwheel_t[fw_count];
		
		// Configure WFS's
		for (int i=0; i<wfs_count; i++) {
			io->msg(IO_XNFO, "Configuring wfs %d/%d", i, wfs_count);

			wfs[i].wfstype = (wfstype_t) cfgfile->getint(format("wfs[%d].type", i));
			wfs[i].name = cfgfile->getstring(format("wfs[%d].name", i), format("WFS #%d", i));

			// Each WFS needs a specific WFS configuration file and a camera config file (can be the same)
			wfs[i].wfscfg = cfgfile->getstring(format("wfs[%d].wfscfg", i));
			wfs[i].camcfg = cfgfile->getstring(format("wfs[%d].camcfg", i));
		}
		
		// Configure WFC's
		for (int i=0; i<wfc_count; i++) {
			io->msg(IO_XNFO, "Configuring wfc %d/%d", i, wfc_count);

			wfc[i].wfctype = (wfctype_t) cfgfile->getint(format("wfc[%d].type", i));
			wfc[i].name = cfgfile->getstring(format("wfc[%d].name", i));
			wfc[i].gain.p = cfgfile->getdouble(format("wfc[%d].gain_p", i));
			wfc[i].gain.i = cfgfile->getdouble(format("wfc[%d].gain_i", i));
			wfc[i].gain.d = cfgfile->getdouble(format("wfc[%d].gain_d", i));
			wfc[i].nact = cfgfile->getint(format("wfc[%d].nact", i));
			wfc[i].wfccfg = cfgfile->getstring(format("wfc[%d].wfccfg", i));
		}
	} catch (exception &e) {
		return io->msg(IO_ERR, "Could not parse configuration: %s", e.what());
	}
	
	io->msg(IO_INFO, "Parsed control configuration.");

	return 0;
}

int foamctrl::verify() {
	int ret=0;
	for (int n=0; n < wfs_count; n++)
		ret += wfs[n].verify();
	for (int n=0; n < wfc_count; n++)
		ret += wfc[n].verify();
	
	return ret;
}
