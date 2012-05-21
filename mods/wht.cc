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
#include "serial.h"

#include "devices.h"
#include "telescope.h"
#include "wht.h"

using namespace std;

WHT::WHT(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Telescope(io, ptc, name, wht_type, port, conffile, online)
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
	if (!sock_track.is_connected()) {
		sock_track.connect(track_host, track_port);
		sock_track.setblocking(true);
	}
	// Open URL
	sock_track.write(format("GET %s HTTP/1.1\r\nHOST %s\r\nUser-Agent: FOAM\r\n\r\n", 
								track_file.c_str(), track_host.c_str()));

	// Read data
	string rawdata;
	printf("Got data (<<EOF):\n");
	while (sock_track.readline(rawdata)) {
		printf("%s\n",rawdata.c_str());
	}
	printf("EOF\n");
	
	// Parse data, find first line after \r\n\r\n
	size_t dbeg = rawdata.find("\r\n\r\n");
	string track_data = rawdata.substr(dbeg);
	
	// Set c0, c1
	*c0 = 0.0;
	*c1 = 1.0;
	
	return 0;
}

void WHT::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {
		string what = popword(line);
		
		if (what == "track") {					// get track
			conn->addtag("track");
			conn->write("ok track " +track_prot+"://"+track_host+":"+track_port+"/"+track_file);
		} else 
			parsed = false;
	} else if (command == "set") {
		string what = popword(line);
		
		parsed = false;
	} else {
		parsed = false;
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Telescope::on_message(conn, orig);
}
