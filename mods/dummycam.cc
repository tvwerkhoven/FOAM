/*
 dummy.cc -- Dummy camera modules
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2006  Guus Sliepen <guus@sliepen.eu.org>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/time.h>
#include <time.h>
#include <math.h>

#include "cam.h"
#include "pthread++.h"

const string dummycam_type "dummycam";

using namespace std;

DummyCamera(Io &io, string name, string port, conffile): 
Camera(io, name, dummycam_type, port, conffile)
{
	io->msg(IO_DEB2, "DummyCamera::DummyCamera(config &config)");
	
	string type = config.getstring("type");
	if (type != DUMMYCAM_TYPE) throw exception("Type should be " DUMMYCAM_TYPE " for this class.");
	
	res.x = config.getint("width", 512);
	res.y = config.getint("height", 512);
	noise = config.getdouble("noise", 0.001);
	
	bpp = 16;
	interval = 0.25;
	exposure = 0.3;
	
	dtype = DATA_UINT16;
	mode = Camera::OFF;
	
	frame = (uint16_t *)malloc(res.x * res.y * bpp/8);
	
	if(!frame)
		throw exception("Could not allocate memory for framebuffer");
	
	io->msg(IO_INFO, "DummyCamera init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
					res.x, res.y, bpp, noise, interval, exposure);
}

void update(bool blocking) {
	io->msg(IO_DEB2, "DummyCamera::update()");
	if(blocking)
		usleep(interval * 1000000);
	
	uint16_t *p = frame;
	int mul = (1 << bpp) - 1;
	for(int y = 0; y < res.y; y++) {
		for(int x = 0; x < res.x; x++) {
			double value = drand48() * noise + (sin(M_PI * x / res.x) + 1 + sin((y + offset) * 100));
			value *= exposure;
			if(value < 0)
				value = 0;
			if(value > 1)
				value = 1;
			*p++ = (uint16_t)(value * mul) & mul;
		}
	}
	offset++;
	if(offset > 1000)
		offset = 0;
}

bool thumbnail(uint8_t *out) {
	update(false);
	
	pthread::mutexholder h(&mutex);
	
	uint16_t *in = frame;
	uint8_t *p = out;
	for(int y = 0; y < 32; y++)
		for(int x = 0 ; x < 32; x++)
			*p++ = in[res.x * y * (res.y / 32) + x * (res.x / 32)] >> (bpp - 8);
	
	return true;
}

bool monitor(void *out, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) {
	if(x1 < 0)
		x1 = 0;
	if(y1 < 0)
		y1 = 0;
	if(scale < 1)
		scale = 1;
	if(x2 * scale > res.x)
		x2 = res.x / scale;
	if(y2 * scale > res.y)
		y2 = res.y / scale;
	
	update(true);
	
	pthread::mutexholder h(&mutex);
	
	uint16_t *data = frame;
	uint16_t *p = (uint16_t *)out;
	
	for(int y = y1 * scale; y < y2 * scale; y += scale) {
		for(int x = x1 * scale; x < x2 * scale; x += scale) {
			*p++ = data[y * res.y + x];
		}
	}
	
	size = (p - (uint16_t *)out) * bpp/8;
	return true;
}
bool capture() {
	update(true);
	
	pthread::mutexholder h(&mutex);
	
	size_t size = res.x * res.y * bpp/8;
	if(write(fd, frame, size != size))
		return false;
	
	return true;
}
