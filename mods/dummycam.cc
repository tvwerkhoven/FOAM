/*
 dummy.cc -- Dummy camera modules
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
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
#include "config.h"
#include "path++.h"
#include "io.h"

#include "camera.h"
#include "dummycam.h"

using namespace std;

DummyCamera::DummyCamera(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Camera(io, ptc, name, dummycam_type, port, conffile, online),
noise(0.001)
{
	io.msg(IO_DEB2, "DummyCamera::DummyCamera()");

	// Register network commands with base device:
	add_cmd("hello world");

	noise = cfg.getdouble("noise", 0.001);
	depth = 16;
	
	set_filename("dummycam-"+name);
	
	io.msg(IO_INFO, "DummyCamera init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
					res.x, res.y, depth, noise, interval, exposure);
	
	// Start camera thread
	mode = Camera::OFF;
	cam_thr.create(sigc::mem_fun(*this, &DummyCamera::cam_handler));
}

DummyCamera::~DummyCamera() {
	io.msg(IO_DEB2, "DummyCamera::~DummyCamera()");
	
	// Delete frames in buffer
	for (size_t f=0; f<nframes; f++) {
		free((uint16_t *) frames[f].data);
		delete[] frames[f].histo;
	}
	
	cam_thr.cancel();
	cam_thr.join();
}

void DummyCamera::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "hello") {						// hello ...
		string what = popword(line);
		
		if(what == "world") {							// hello world
			io.msg(IO_DEB1, "DummyCamera::on_message(): hello world!!!"); 
			conn->write("ok :hello world back!");
		} 
		else
			parsed = false;
	} 
	else
		parsed = false;
	
	// If not parsed here, call parent
	if (parsed == false)
		Camera::on_message(conn, orig);
}


void DummyCamera::update() {
	io.msg(IO_DEB2, "DummyCamera::update()");
		
	static struct timeval now, next, diff;
	
	gettimeofday(&now, 0);
	
	// Allocate new memory for this frame.
	uint16_t *image = (uint16_t *) malloc(res.x * res.y * sizeof *image);
	if (!image)
		throw exception("DummyCamera::update(): Could not allocate memory for framebuffer");	

	uint16_t *p = image;
	
	int mul = (1 << depth) - 1;
	for(size_t y = 0; y < (size_t) res.y; y++) {
		for(size_t x = 0; x < (size_t) res.x; x++) {
			double value = simple_rand() * noise + (sin(M_PI * x / res.x) + 1 + sin((y + offset) * 100));
			value *= exposure;
			if(value < 0)
				value = 0;
			if(value > 1)
				value = 1;
			*p++ = (uint16_t)(value * mul) & mul;
		}
	}
	
	// Queue this frame, if the buffer is full, we will get the oldest one back
	void *old = cam_queue(image, image, &now);
	
	if (old) {
		io.msg(IO_DEB2, "DummyCamera::update(): got old frame=%p", old);
		free((uint16_t *) old);
	}
	
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
			case Camera::WAITING:
				io.msg(IO_INFO, "DummyCamera::cam_handler() OFF/WAITING.");
				// We wait until the mode changed
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

void DummyCamera::cam_set_exposure(const double value) {
	pthread::mutexholder h(&cam_mutex);
	exposure = value;
}

double DummyCamera::cam_get_exposure() {
	return exposure;
}

void DummyCamera::cam_set_interval(const double value) {
	pthread::mutexholder h(&cam_mutex);
	interval = value;
}

double DummyCamera::cam_get_interval() {
	return interval;
}

void DummyCamera::cam_set_gain(const double value) {
	pthread::mutexholder h(&cam_mutex);
	gain = value;
}

double DummyCamera::cam_get_gain() {
	return gain;
}

void DummyCamera::cam_set_offset(const double value) {
	pthread::mutexholder h(&cam_mutex);
	offset = value;
}

double DummyCamera::cam_get_offset() {
	return offset;
}

void DummyCamera::cam_set_mode(const mode_t newmode) {
	pthread::mutexholder h(&cam_mutex);
	if (newmode == mode)
		return;
	
	mode = newmode;
	mode_cond.broadcast();
}
