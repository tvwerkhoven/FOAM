/*
 cam.cc -- generic camera input/output wrapper
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
/*! 
 @file cam.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl) and Guus Sliepen (guus@sliepen.org)
 @brief Generic camera class.
 
 This class extends the Device class and provides a base class for cameras. Does not implement anything itself.
 */

#include <string>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "io.h"
#include "types.h"
#include "config.h"

#include "devices.h"
#include "camera.h"

using namespace std;

Camera::Camera(Io &io, string name, string type, string port, string conffile): 
Device(io, name, cam_type + "." + type, port, conffile),
nframes(8), timeouts(0), ndark(10), nflat(10), 
interval(1.0), exposure(1.0), gain(1.0), offset(0.0), 
res(0,0), depth(-1), dtype(DATA_UINT16),
mode(Camera::OFF)
{
	io.msg(IO_DEB2, "Camera::Camera()");
	
	nframes = cfg.getint("nframes", 8);
	frames = new frame_t[nframes];
	
	ndark = cfg.getint("ndark", 10);
	nflat = cfg.getint("nflat", 10);
	
	interval = cfg.getdouble("interval", 1.0);
	exposure = cfg.getdouble("exposure", 1.0);
	gain = cfg.getdouble("gain", 1.0);
	offset = cfg.getdouble("offset", 0.0);
	
	res.x = cfg.getint("width", 512);
	res.y = cfg.getint("height", 512);
	depth = cfg.getint("depth", 8);
	
}

Camera::~Camera() {
	delete frames;
}

void Camera::calculate_stats(frame *frame) {
	memset(frame->histo, 0, get_maxval() * sizeof *frame->histo);
	
	if(depth <= 8) {
		uint8_t *image = (uint8_t *)frame->image;
		for(size_t i = 0; i < res.x * res.y; i++) {
			frame->histo[image[i]]++;
		}
	} else {
		uint16_t *image = (uint16_t *)frame->image;
		for(size_t i = 0; i < res.x * res.y; i++) {
			frame->histo[image[i]]++;
		}
	}
	
	double sum = 0;
	double sumsquared = 0;
	
	for(size_t i = 0; i < get_maxval(); i++) {
		sum += (double)i * frame->histo[i];
		sumsquared += (double)(i * i) * frame->histo[i];
	}
	
	sum /= res.x * res.y;
	sumsquared /= res.x * res.y;
	
	frame->avg = sum;
	frame->rms = sqrt(sumsquared - sum * sum) / sum;
}

// Network IO goes here

void Camera::on_message(conn *conn, string line) {
	string command = popword(line);
	
	if (command == "quit" || command == "exit") {
		conn->write("OK :Bye!");
		conn->close();
	} else if (command == "restart") {
		conn->write("OK");
		do_restart();
	} else if (command == "set") {
		string what = popword(line);
		
		//! @todo what is mode?
		if(what == "mode")
			set_mode(conn, popword(line));
		else if(what == "exposure")
			set_exposure(conn, popdouble(line));
		else if(what == "interval")
			set_interval(conn, popdouble(line));
		else if(what == "gain")
			set_gain(conn, popdouble(line));
		else if(what == "offset")
			set_offset(conn, popdouble(line));
		else if(what == "filename")
			set_filename(conn, popword(line));
		else if(what == "outputdir")
			set_outputdir(conn, popword(line));
		else if(what == "fits")
			set_fits(conn, line);
		else
			conn->write("ERROR :Unknown argument " + what);
	} else if (command == "get") {
		string what = popword(line);
		
		if(what == "mode") {
			conn->addtag("mode");
			get_mode(conn);
		} else if(what == "exposure") {
			conn->addtag("exposure");
			conn->write(format("OK exposure %lf", exposure));
		} else if(what == "interval") {
			conn->addtag("interval");
			conn->write(format("OK interval %lf", interval));
		} else if(what == "gain") {
			conn->addtag("gain");
			conn->write(format("OK gain %lf", gain));
		} else if(what == "offset") {
			conn->addtag("offset");
			conn->write(format("OK offset %lf", offset));
		} else if(what == "width") {
			conn->write(format("OK width %d", width));
		} else if(what == "height") {
			conn->write(format("OK height %d", height));
		} else if(what == "depth") {
			conn->write(format("OK depth %d", depth));
		} else if(what == "state") {
			conn->addtag("state");
			get_state(conn);
		} else if(what == "filename") {
			conn->addtag("filename");
			conn->write("OK filename :" + filenamebase);
		} else if(what == "outputdir") {
			conn->addtag("outputdir");
			conn->write("OK outputdir :" + outputdir);
		} else if(what == "fits") {
			get_fits(conn);
		} else {
			conn->write("ERROR :Unknown argument " + what);
		}
	} else if (command == "thumbnail") {
		thumbnail(conn);
	} else if (command == "grab") {
		int x1 = popint(line);
		int y1 = popint(line);
		int x2 = popint(line);
		int y2 = popint(line);
		int scale = popint(line);
		bool do_df = false;
		bool do_histo = false;
		string option;
		while(!(option = popword(line)).empty()) {
			if(option == "darkflat")
				do_df = true;
			else if(option == "histogram")
				do_histo = true;
		}
		
		grab(conn, x1, y1, x2, y2, scale, do_df, do_histo);
	} else if(command == "dark") {
		darkburst(conn, popint(line));
	} else if(command == "flat") {
		flatburst(conn, popint(line));
	} else if(command == "statistics") {
		statistics(conn, popint(line));
	} else {
		conn->write("ERROR :Unknown command");
	}
}

