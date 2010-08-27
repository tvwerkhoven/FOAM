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
	
	set_filename("dummycam-"+name);
	
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

void DummyCamera::update() {
	io.msg(IO_DEB2, "DummyCamera::update()");
		
	static struct timeval now, next, diff;
	
	gettimeofday(&now, 0);
	
	uint16_t *image = (uint16_t *) malloc(res.x * res.y * depth/8);
	if (!image)
		throw exception("DummyCamera::update(): Could not allocate memory for framebuffer");	

	uint16_t *p = image;
	
	int mul = (1 << depth) - 1;
	for(size_t y = 0; y < res.y; y++) {
		for(size_t x = 0; x < res.x; x++) {
			double value = drand48() * noise + (sin(M_PI * x / res.x) + 1 + sin((y + offset) * 100));
			value *= exposure;
			if(value < 0)
				value = 0;
			if(value > 1)
				value = 1;
			*p++ = (uint16_t)(value * mul) & mul;
		}
	}
	
	void *old = cam_queue(image, image, &now);
	if(old)
		free(old);
	
	// Make sure each update() takes at minimum interval seconds:
	diff.tv_sec = 0;
	diff.tv_usec = interval * 1.0e6;
	timeradd(&now, &diff, &next);
	
	gettimeofday(&now, 0);
	timersub(&next, &now, &diff);
	if(diff.tv_sec >= 0)
		usleep(diff.tv_sec * 1.0e6 + diff.tv_usec);
}

void DummyCamera::do_restart() {
	io.msg(IO_INFO, "DummyCamera::do_restart()");
}

void DummyCamera::cam_handler() { 
	pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);
	sleep(1);
	
	while (true) {
		switch (mode) {
			case Camera::RUNNING:
				io.msg(IO_DEB1, "DummyCamera::cam_handler() RUNNING");
				update();
				break;
			case Camera::SINGLE:
				io.msg(IO_DEB1, "DummyCamera::cam_handler() SINGLE");
				update();
				mode = Camera::OFF;
				break;
			case Camera::OFF:
				io.msg(IO_INFO, "DummyCamera::cam_handler() OFF.");
				// We wait until the mode changed
				//! @todo Is this correct?
				mode_mutex.lock();
				mode_cond.wait(mode_mutex);
				mode_mutex.unlock();
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

void DummyCamera::cam_set_exposure(double value) {
	pthread::mutexholder h(&cam_mutex);
	exposure = value;
}

void DummyCamera::cam_set_interval(double value) {
	pthread::mutexholder h(&cam_mutex);
	interval = value;
}

void DummyCamera::cam_set_gain(double value) {
	pthread::mutexholder h(&cam_mutex);
	gain = value;
}

void DummyCamera::cam_set_offset(double value) {
	pthread::mutexholder h(&cam_mutex);
	offset = value;
}

void DummyCamera::cam_set_mode(const mode_t newmode) {
	pthread::mutexholder h(&cam_mutex);
	if (newmode == mode)
		return;
	
	mode = newmode;
	mode_cond.broadcast();
}