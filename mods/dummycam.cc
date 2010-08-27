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

#include "pthread++.h"

#include "camera.h"
#include "dummycam.h"

using namespace std;

const std::string dummycam_type = "dummycam";

DummyCamera::DummyCamera(Io &io, foamctrl *ptc, string name, string port, string conffile): 
Camera(io, ptc, name, dummycam_type, port, conffile)
{
	io.msg(IO_DEB2, "DummyCamera::DummyCamera()");

	noise = cfg.getdouble("noise", 0.001);
	
	depth = 16;
	interval = 0.25;
	exposure = 0.3;
	
	dtype = DATA_UINT16;
	mode = Camera::OFF;
	
	io.msg(IO_INFO, "DummyCamera init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
					res.x, res.y, depth, noise, interval, exposure);
	
	// Start camera thread
	cam_thr.create(sigc::mem_fun(*this, &DummyCamera::cam_handler));
}

DummyCamera::~DummyCamera() {
	io.msg(IO_DEB2, "DummyCamera::~DummyCamera()");
	cam_thr.cancel();
	cam_thr.join();
}

void DummyCamera::update(bool blocking) {
	io.msg(IO_DEB2, "DummyCamera::update()");
	if (blocking)
		usleep(interval * 1000000);
	
	uint16_t *p = (uint16_t *) malloc(res.x * res.y * depth/8);
	if (!p)
		throw exception("DummyCamera::update(): Could not allocate memory for framebuffer");	
	
	int mul = get_maxval();
	
	for (size_t y = 0; y < res.y; y++) {
		for (size_t x = 0; x < res.x; x++) {
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
	
	uint16_t *old  = (uint16_t *) cam_queue(p, p);
	if (old)
		free(old);
}

void *DummyCamera::cam_queue(void *data, void *image, struct timeval *tv) {
	io.msg(IO_DEB2, "DummyCamera::cam_queue()");
	
	pthread::mutexholder h(&cam_mutex);
	
	frame_t *frame = &frames[count % nframes];
	void *old = frame->data;
	frame->data = data;
	frame->image = image;
	frame->id = count++;
	
	if(!frame->histo)
		frame->histo = new uint32_t[get_maxval()];
	
	calculate_stats(frame);
	
	if(tv)
		frame->tv = *tv;
	else
		gettimeofday(&frame->tv, 0);
	io.msg(IO_DEB2, "DummyCamera::cam_queue(): %8zu %p %p %p %7.3lf %6.3lf", count, frame, data, image, frame->avg, frame->rms);
		
	cam_cond.broadcast();
	
	// Return the oldest frame if the ringbuffer is full, else return NULL
	return old;
}

bool DummyCamera::thumbnail(uint8_t *out) {
	io.msg(IO_DEB2, "DummyCamera::thumbnail()");
	update(false);
	
	pthread::mutexholder h(&cam_mutex);
	
	uint16_t *in = (uint16_t *) frames[count % nframes].data;
	uint8_t *p = out;
	for (size_t y = 0; y < 32; y++)
		for (size_t x = 0; x < 32; x++)
			*p++ = in[res.x * y * (res.y / 32) + x * (res.x / 32)] >> (depth - 8);
	
	return true;
}

bool DummyCamera::monitor(void *out, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) {
	io.msg(IO_DEB2, "DummyCamera::monitor()");
	
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
	
	pthread::mutexholder h(&cam_mutex);
	
	uint16_t *data = (uint16_t *) frames[count % nframes].data;
	uint16_t *p = (uint16_t *)out;
	
	for(int y = y1 * scale; y < y2 * scale; y += scale) {
		for(int x = x1 * scale; x < x2 * scale; x += scale) {
			*p++ = data[y * res.y + x];
		}
	}
	
	size = (p - (uint16_t *)out) * depth/8;
	return true;
}

void DummyCamera::cam_handler() { 
	while (true) {
		switch (mode) {
			case Camera::OFF:
				io.msg(IO_INFO, "DummyCamera::cam_handler() OFF.");
				// We wait until the mode changed
				//! @todo Is this correct?
				cam_mutex.lock();
				cam_cond.wait(cam_mutex);
				cam_mutex.unlock();
				break;
			case Camera::SINGLE:
				io.msg(IO_DEB1, "DummyCamera::cam_handler() SINGLE");
				update(true);
				mode = Camera::OFF;
				break;
			case Camera::RUNNING:
				io.msg(IO_DEB1, "DummyCamera::cam_handler() RUNNING");
				update(true);
				break;
			case Camera::CONFIG:
				io.msg(IO_DEB1, "DummyCamera::cam_handler() CONFIG");
				break;
			default:
				io.msg(IO_ERR, "DummyCamera::cam_handler() UNKNOWN!");
				break;
		}
		
	}
}
