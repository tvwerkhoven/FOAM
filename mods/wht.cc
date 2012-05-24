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
Telescope(io, ptc, name, wht_type, port, conffile, online),
wht_ctrl(NULL), sport(""),
alt(0.0), az(0.0)
{
	io.msg(IO_DEB2, "WHT::WHT()");
	
	// Configure initial settings
	{
		// port
		sport = cfg.getstring("port");
		
		// delimiter
		// coords url = http://whtics.roque.ing.iac.es:8081/TCSStatus/TCSStatusExPo
		track_prot = "http://";
		track_host = cfg.getstring("track_host", "whtics.roque.ing.iac.es");
		track_port = cfg.getstring("track_port", "8081");
		track_file = cfg.getstring("track_file", "/TCSStatus/TCSStatusExPo");
	}
	
	//wht_ctrl = new serial::port(sport, B9600, 0, '\r');
}

WHT::~WHT() {
	io.msg(IO_DEB2, "WHT::~WHT()");

	//!< @todo Save all device settings back to cfg file
}

int WHT::update_wht_coords(double *const alt, double *const az) {
	// Connect if necessary
	if (!sock_track.is_connected()) {
		io.msg(IO_DEB1, "WHT::update_wht_coords(): connecting to %s:%s...", track_host.c_str(), track_port.c_str());
		sock_track.connect(track_host, track_port);
		sock_track.setblocking(false);
	}

	// Open URL, request disconnect 
	sock_track.printf("GET %s HTTP/1.1\r\nHOST: %s\r\nUser-Agent: FOAM dev.telescope.wht\r\nConnection: close\r\n\r\n\n", 
										track_file.c_str(), track_host.c_str());

	// Read data
	string rawdata;
	char buf[2048];
	while (sock_track.read((void *)buf, 2048))
		rawdata += string(buf);

	// Parse data, find first line after \r\n\r\n
	size_t dbeg = rawdata.find("\r\n\r\n") +4;
	if (dbeg == string::npos) {
		io.msg(IO_WARN, "WHT::update_wht_coords(): could not find data.");
		return -1;
	}
	string track_data = rawdata.substr(dbeg);
	io.msg(IO_DEB1, "WHT::update_wht_coords(): data @ %zu: %s...", dbeg, track_data.substr(0,30).c_str());
	
	// Split by words, find coordinates, store and return
	string key, val;
	while (track_data.length()) {
		key = popword(track_data, "=");
		val = popword(track_data, "\n");
		wht_info[key] = val;
		io.msg(IO_DEB1, "WHT::update_wht_coords(): got: %s=%s", key.c_str(), val.c_str());
	}
	
	// Check if we got the ALT and AZ parameters
	if (wht_info.find("AZ") == wht_info.end() or wht_info.find("ALT") == wht_info.end()) {		
		io.msg(IO_WARN, "WHT::update_wht_coords(): did not get alt/az information!");
	} else {
		// Set elevation, declination
		*alt = strtod(wht_info["ALT"].c_str(), NULL);
		*az = strtod(wht_info["AZ"].c_str(), NULL);
		io.msg(IO_XNFO, "WHT::update_wht_coords(): new alt=%g, az=%g", *alt, *az);
	}
	
	return 0;
}

int WHT::update_telescope_track(const float sht0, const float sht1) {
	io.msg(IO_DEB1, "WHT::update_telescope_track(sht0=%g, sht1=%g)", sht0, sht1);
	static int iter = 0;
	// If it's time for a new telescope position, get it
	if (iter % 10 == 0) {
		io.msg(IO_DEB1, "WHT::update_telescope_track() iter=%d, get coords", iter);
		update_wht_coords(&alt, &az);
	}
	
	// We have shift in the focal plane, convert to telescope axis by rotation:
	// General:
	// x' = [ x cos(th) - y sin(th) ]
	// y; = [ x sin(th) + y cos(th) ]
	// For ExPo:
	// ele = 0.001 * sin(??) + cos(??)
	// dec = 0.001 * sin(??) + cos(??)
	float d_ele = sht0 * cos(alt) - sht1 * sin(alt);
	float d_alt = sht0 * sin(alt) + sht1 * cos(alt);
	
	iter++;
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
		} else if (what == "pos") {			// get pos
				conn->addtag("pos");
				conn->write(format("ok pos %d %d", alt, az));
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
