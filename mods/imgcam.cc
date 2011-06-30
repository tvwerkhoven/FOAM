/*
 imgcam.cc -- Dummy 'camera' with static images as source
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <fcntl.h>
#include <stdint.h>

#include "config.h"
#include "io.h"
#include "imgdata.h"

#include "imgcam.h"

using namespace std;


ImgCamera::ImgCamera(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Camera(io, ptc, name, imgcam_type, port, conffile, online),
noise(10.0), img(NULL), frame(NULL)
{
	io.msg(IO_DEB2, "ImgCamera::ImgCamera()");
	// Register network commands with base device:
	// No extra commands

	Path file = cfg.getstring("imagefile");
	file = ptc->confdir + file;
	
	io.msg(IO_DEB2, "imagefile = %s", file.c_str());
	noise = cfg.getdouble("noise", 10.0);
	interval = cfg.getdouble("interval", 0.25);
	exposure = cfg.getdouble("exposure", 1.0);
	
	mode = Camera::OFF;
	
	img = new ImgData(io, file, ImgData::FITS);
	if (img->geterr() != ImgData::ERR_NO_ERROR)
		throw exception("ImgCamera::ImgCamera(): Could not load image");

	img->calcstats();
	
	res.x = img->getwidth();
	res.y = img->getheight();
	depth = img->getbpp();
	
	frame = (uint16_t *) malloc(res.x * res.y * depth/8);
	
	update();
	
	io.msg(IO_INFO, "ImgCamera: init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
				 res.x, res.y, depth, noise, interval, exposure);
	if (img->stats.init)
		io.msg(IO_INFO, "ImgCamera: Range = %d--%d, sum=%lld", img->stats.min, img->stats.max, img->stats.sum);
}

ImgCamera::~ImgCamera() {
	io.msg(IO_DEB2, "ImgCamera::~ImgCamera()");
	delete img;
	free(frame);
}

void ImgCamera::update() {
	io.msg(IO_DEB2, "ImgCamera::update()");
	
	static struct timeval now, next, diff;
	
	gettimeofday(&now, 0);
	
	uint16_t *p = frame;

	// Only process frame if necessary...
	if (noise != 0 & exposure != 0) {
		int mul = (1 << depth) - 1;
		for(size_t y = 0; y < (size_t) res.y; y++) {
			for(size_t x = 0; x < (size_t) res.x; x++) {
				double value = simple_rand() * noise + img->getpixel(x, y) * exposure;
				value *= exposure;
				if(value < 0)
					value = 0;
				if(value > 1)
					value = 1;
				*p++ = (uint16_t)(value * mul) & mul;
			}
		}
	}
	
	void *old = cam_queue(frame, frame, &now);
	
	if (interval > 0) {
		// Make sure each update() takes at minimum interval seconds:
		diff.tv_sec = 0;
		diff.tv_usec = interval * 1.0e6;
		timeradd(&now, &diff, &next);
		
		gettimeofday(&now, 0);
		timersub(&next, &now, &diff);
		if(diff.tv_sec >= 0)
			usleep(diff.tv_sec * 1.0e6 + diff.tv_usec);
	}
}

void ImgCamera::cam_handler() { 
	pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);
	
	while (true) {
		switch (mode) {
			case Camera::RUNNING:
				io.msg(IO_DEB1, "ImgCamera::cam_handler() RUNNING");
				update();
				break;
			case Camera::SINGLE:
				io.msg(IO_DEB1, "ImgCamera::cam_handler() SINGLE");
				update();
				mode = Camera::OFF;
				break;
			case Camera::OFF:
			case Camera::CONFIG:
			default:
				io.msg(IO_INFO, "ImgCamera::cam_handler() OFF/CONFIG/UNKNOWN");
				// We wait until the mode changed
				{
					pthread::mutexholder h(&mode_mutex);
					mode_cond.wait(mode_mutex);
				}				
				break;
		}
	}
}

void ImgCamera::cam_set_exposure(const double value) {
	pthread::mutexholder h(&cam_mutex);
	exposure = value;
}

double ImgCamera::cam_get_exposure() {
	return exposure;
}

void ImgCamera::cam_set_interval(const double value) {
	pthread::mutexholder h(&cam_mutex);
	interval = value;
}

double ImgCamera::cam_get_interval() {
	return interval;
}

void ImgCamera::cam_set_gain(const double value) {
	pthread::mutexholder h(&cam_mutex);
	gain = value;
}

double ImgCamera::cam_get_gain() {
	return gain;
}

void ImgCamera::cam_set_offset(const double value) {
	pthread::mutexholder h(&cam_mutex);
	offset = value;
}

double ImgCamera::cam_get_offset() {
	return offset;
}

void ImgCamera::cam_set_mode(const mode_t newmode) {
	pthread::mutexholder h(&cam_mutex);
	if (newmode == mode)
		return;
	
	mode = newmode;
	{
		pthread::mutexholder h(&mode_mutex);
		mode_cond.broadcast();
	}				
}

void ImgCamera::do_restart() {
	io.msg(IO_INFO, "ImgCamera::do_restart()");
}

void ImgCamera::on_message(Connection *const conn, string line) {
	Camera::on_message(conn, line);
}
