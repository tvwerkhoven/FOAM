/*
 andor.cc -- Andor iXON camera modules
 Copyright (C) 2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

AndorCam::AndorCam(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Camera(io, ptc, name, andor_type, port, conffile, online),
cooltemp(30), cooler_on(false)
{
	io.msg(IO_DEB2, "AndorCam::AndorCam()");
	
	// Register network commands here
	add_cmd("set cooling");
	add_cmd("get cooling");
	
	// Get configuration parameters
	cooltemp = cfg.getint("cooltemp", 30);
	// res
	// interval
	// exposure
	// gain
	// depth = 16;
	
	// Set filename prefix for saved frames
	set_filename("andorcam-"+name);
	
	// Initialize default configuration
	cam_get_coolrange();
	cam_set_cooltemp(cooltemp);
	
	io.msg(IO_INFO, "AndorCam init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
				 res.x, res.y, depth, noise, interval, exposure);
	
	// Start camera thread
	mode = Camera::OFF;
	cam_thr.create(sigc::mem_fun(*this, &AndorCam::cam_handler));
}

AndorCam::~AndorCam() {
	io.msg(IO_DEB2, "AndorCam::~AndorCam()");
	
	// Acquisition off
	// Close shutter
	// Warm CCD
	
	// Delete frames in buffer if necessary

	// Stop capture thread
	cam_thr.cancel();
	cam_thr.join();
}

void AndorCam::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {							// get ...
		string what = popword(line);
		
		if (what == "cooling") {					// get cooling
			conn->addtag("cooling");
			conn->write(format("ok cooling %d", cooltemp));
		} 
		else
			parsed = false;
	} else if (comand = "set") {				// set ...
		string what = popword(line);

		if (what == "cooling") {					// set cooling <temp>
			int temp = popint(line);
			conn->addtag("cooling");
			if (temp < coolrange[0] && temp > coolrange[1])
				cam_set_cooltemp(temp);
			else
				conn->write("error :temperature invalid, should be [%d, %d]", coolrange[0], coolrange[1]);
		}
	} else
		parsed = false;
	
	// If not parsed here, call parent
	if (parsed == false)
		Camera::on_message(conn, orig);
}

void AndorCam::do_restart() {
	io.msg(IO_INFO, "AndorCam::do_restart()");
}

void AndorCam::cam_handler() { 
	pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);
	sleep(1);
	
	while (true) {
		switch (mode) {
			case Camera::RUNNING:
				io.msg(IO_DEB1, "AndorCam::cam_handler() RUNNING");
				//! @todo Implement data acquisition in Andor SDK
				// open shutter
				// start acquisition
				
				break;
			case Camera::SINGLE:
				io.msg(IO_DEB1, "AndorCam::cam_handler() SINGLE");
				//! @todo Implement data acquisition in Andor SDK
				mode = Camera::OFF;
				break;
			case Camera::OFF:
			case Camera::WAITING:
				io.msg(IO_INFO, "AndorCam::cam_handler() OFF/WAITING.");
				// stop acquisition
				// close shutter

				// We wait until the mode changed
				mode_mutex.lock();
				mode_cond.wait(mode_mutex);
				mode_mutex.unlock();
				break;
			case Camera::CONFIG:
				io.msg(IO_DEB1, "AndorCam::cam_handler() CONFIG");
				break;
			default:
				io.msg(IO_ERR, "AndorCam::cam_handler() UNKNOWN!");
				break;
		}
		
	}
}

void AndorCam::cam_set_exposure(const double value) {
	pthread::mutexholder h(&cam_mutex);
	//! @todo Implement in Andor SDK
#error not implemented
}

double AndorCam::cam_get_exposure() {
	//! @todo Implement in Andor SDK
#error not implemented
}

void AndorCam::cam_set_interval(const double value) {
	pthread::mutexholder h(&cam_mutex);
	//! @todo Implement in Andor SDK
#error not implemented
}

double AndorCam::cam_get_interval() {
	//! @todo Implement in Andor SDK
#error not implemented
}

void AndorCam::cam_set_gain(const double value) {
	pthread::mutexholder h(&cam_mutex);
	//! @todo Implement in Andor SDK
#error not implemented
}

double AndorCam::cam_get_gain() {
	//! @todo Implement in Andor SDK
#error not implemented
}

void AndorCam::cam_set_offset(const double value) {
	pthread::mutexholder h(&cam_mutex);
	//! @todo Implement in Andor SDK
#error not implemented
}

double AndorCam::cam_get_offset() {
	//! @todo Implement in Andor SDK
#error not implemented
}

void AndorCam::cam_set_mode(const mode_t newmode) {
	pthread::mutexholder h(&cam_mutex);
	if (newmode == mode)
		return;
	
	mode = newmode;
	mode_cond.broadcast();
	
	//! @todo Implement in Andor SDK
#error not implemented
}

void AndorCam::cam_get_coolrange() {
	if (GetTemperatureRange(&(coolrange[0]), &(coolrange[1])) != DRV_SUCCESS)
		coolrange[0] = coolrange[1] = 0;
}

int AndorCam::cam_set_cooltemp(const int value) {	
	//! @todo implement error on CoolerON() failure
	if (CoolerON() != DRV_SUCCESS)
		return cooltemp;
	
	cooler_on = true;
	
	if (SetTemperature(value) == DRV_SUCCESS)
		cooltemp = value;
	
	netio.broadcast(format("ok cooling %lf", cooltemp), "cooling");
	return cooltemp;	
}
