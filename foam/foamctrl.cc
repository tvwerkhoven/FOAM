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

	for (int i=0; i<wfs_count; i++) delete wfs[i];
	for (int i=0; i<wfc_count; i++) delete wfc[i];
	
	delete[] wfs;
	delete[] wfc;
	delete[] filter;
	delete[] wfscfgs;
	delete[] wfccfgs;
	delete cfgfile;
}

foamctrl::foamctrl(void) { 
	io->msg(IO_DEB2, "foamctrl::foamctrl(void)");
	starttime = time(NULL); 
	frames = 0; 
	wfs_count = wfc_count = fw_count = 0;
	err = 0;
}

foamctrl::foamctrl(string &file) {
	io->msg(IO_DEB2, "foamctrl::foamctrl()");
	
	starttime = time(NULL); 
	frames = 0; 
	wfs_count = wfc_count = fw_count = 0;
	err = 0;
	parse(file);
}

int foamctrl::parse(string &file) {
	io->msg(IO_DEB2, "foamctrl::parse()");

	conffile = file;
	int idx = conffile.find_last_of("/");
	confpath = conffile.substr(0, idx);
	
	io->msg(IO_DEB2, "foamctrl::parse: got file %s and path %s.", conffile.c_str(), confpath.c_str());

	cfgfile = new config(conffile);
	
	mode = AO_MODE_LISTEN;		// start in listen mode
	calmode = CAL_INFL;
	
	// Load AO configuration files now
	try {
		wfs_count = cfgfile->getint("wfs_count");
		wfc_count = cfgfile->getint("wfc_count");
		fw_count = 0;//cfgfile->getint("fw_count");
		
		// Allocate memory for configuration files
		wfscfgs = new string[wfs_count];
		wfccfgs = new string[wfc_count];
		
		wfs = new Wfs*[wfs_count];
		wfc = new Wfc*[wfc_count];
		filter = new fwheel_t[fw_count];
		
		// Get WFS configuration files
		for (int i=0; i<wfs_count; i++) {
			io->msg(IO_XNFO, "Configuring wfs %d/%d", i+1, wfs_count);
			wfscfgs[i] = confpath + "/" + cfgfile->getstring(format("wfs[%d].cfg", i));
		}
		
		// Get WFC configuration files
		for (int i=0; i<wfc_count; i++) {
			io->msg(IO_XNFO, "Configuring wfc %d/%d", i+1, wfc_count);
			wfccfgs[i] = confpath + "/" + cfgfile->getstring(format("wfc[%d].cfg", i));
		}
	
	} catch (exception &e) {
		err = 1;
		return io->msg(IO_ERR, "Could not parse configuration: %s", e.what());
	}
	
	io->msg(IO_INFO, "Successfully parsed control configuration.");

	return 0;
}

int foamctrl::verify() {
	int ret=0;
	for (int n=0; n < wfs_count; n++)
		ret += (wfs[n])->verify();
//	for (int n=0; n < wfc_count; n++)
//		ret += wfc[n]->verify();
	
	return ret;
}
