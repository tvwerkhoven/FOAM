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
