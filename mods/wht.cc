/*
 wht.h -- William Herschel Telescope control
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

#include "telescope.h"
#include "devices.h"

using namespace std;

WHT::WHT(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online):
Telescope(io, ptc, name, telescope_type + "." + type, port, conffile, online),
{
	io.msg(IO_DEB2, "WHT::WHT()");
	
	// Configure initial settings
	{
		// port
		// parity
		// speed
		// delimiter
		// coords url
		track_prot = "http://";
		track_host = cfg.getstring("track_host", "whtics.roque.ing.iac.es");
		track_port = cfg.getstring("track_port", "8081");
		track_file = cfg.getstring("track_port", "/TCSStatus/TCSStatusExPo");
	}
}

WHT::~WHT() {
	io.msg(IO_DEB2, "WHT::~WHT()");

	//!< @todo Save all device settings back to cfg file
}

int WHT::get_wht_coords(float *c0, float *c1) {
	// Connect if necessary
	if (!sock_live.is_connected()) {
		sock_live.connect(track_host, track_port);
		sock_live.setblocking(true);
	}
	// Open URL
	sock_live.write("GET %s HTTP/1.1\r\nHOST %s\r\nUser-Agent: FOAM\r\n\r\n", 
								track_file, track_host);

	// Read data
	string rawdata;
	printf("Got data (<<EOF):\n");
	while (sock_live.readline(rawdata)) {
		printf(rawdata);
	}
	printf("EOF\n");
	
	// Parse data, find first line after \r\n\r\n
	size_t dbeg = rawdata.find("\r\n\r\n");
	string track_data = rawdata.substr(dbeg);
	
	// Set c0, c1
	c0 = 0;
	c1 = 1;
	
	return 0;
}
