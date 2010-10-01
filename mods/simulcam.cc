/*
 simulcam.cc -- atmosphere/telescope simulator camera
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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


#include "io.h"
#include "config.h"
#include "pthread++.h"

#include "simseeing.h"
#include "camera.h"
#include "simulcam.h"

SimulCam::SimulCam(Io &io, foamctrl *ptc, string name, string port, Path &conffile):
Camera(io, ptc, name, SimulCam_type, port, conffile),
seeing(io, ptc, name + "-seeing", port, conffile)
{
	io.msg(IO_DEB2, "SimulCam::SimulCam()");
	//! @todo init seeing, atmosphere, simulator, etc.
	
	cam_thr.create(sigc::mem_fun(*this, &SimulCam::cam_handler));
}


SimulCam::~SimulCam() {
	//! @todo stop simulator etc.
	io.msg(IO_DEB2, "SimulCam::~SimulCam()");
	cam_thr.cancel();
	cam_thr.join();
	
	mode = Camera::OFF;
}

// From Camera::
void SimulCam::cam_set_exposure(double value) {
	pthread::mutexholder h(&cam_mutex);
	exposure = value;
}

double SimulCam::cam_get_exposure() {
	pthread::mutexholder h(&cam_mutex);
	return exposure;
}

void SimulCam::cam_set_interval(double value) {
	pthread::mutexholder h(&cam_mutex);
	interval = value;
}

double SimulCam::cam_get_interval() {
	return interval;
}

void SimulCam::cam_set_gain(double value) {
	pthread::mutexholder h(&cam_mutex);
	gain = value;
}

double SimulCam::cam_get_gain() {
	return gain;
}

void SimulCam::cam_set_offset(double value) {
	pthread::mutexholder h(&cam_mutex);
	offset = value;
}

double SimulCam::cam_get_offset() {	
	return offset;
}

void SimulCam::cam_handler() { 
	pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);
	//! @todo make this Mac compatible
#ifndef __APPLE__
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread::setaffinity(&cpuset);
#endif
	
	while (true) {
		//! @todo Should mutex lock each time reading mode, or is this ok?
		switch (mode) {
			case Camera::RUNNING:
			{
				io.msg(IO_DEB1, "SimulCam::cam_handler() RUNNING");
				//! @todo get frame from SimSeeing, pass it to camera.cc:cam_queue()
				
				// cam_queue(frame, frame->image);
				break;
			}
			case Camera::SINGLE:
			{
				io.msg(IO_DEB1, "SimSeeing::cam_handler() SINGLE");
				//! @todo get frame from SimSeeing, pass it to camera.cc:cam_queue()
				
				// cam_queue(frame, frame->image);
				
				mode = Camera::WAITING;
				break;
			}
			case Camera::OFF:
			case Camera::WAITING:
			case Camera::CONFIG:
			default:
				io.msg(IO_INFO, "SimSeeing::cam_handler() OFF/WAITING/UNKNOWN.");
				// We wait until the mode changed
				mode_mutex.lock();
				mode_cond.wait(mode_mutex);
				mode_mutex.unlock();
				break;
		}
	}
}

void SimulCam::cam_set_mode(mode_t newmode) {
	if (newmode == mode)
		return;
	
	switch (newmode) {
		case Camera::RUNNING:
		case Camera::SINGLE:
			// Start camera
			mode = newmode;
			mode_cond.broadcast();
			break;
		case Camera::WAITING:
			// Stop camera
			mode = newmode;
			mode_cond.broadcast();
			break;
		case Camera::OFF:
		case Camera::CONFIG:
			io.msg(IO_INFO, "SimSeeing::cam_set_mode(%s) mode not supported.", mode2str(newmode).c_str());
		default:
			io.msg(IO_WARN, "SimSeeing::cam_set_mode(%s) mode unknown.", mode2str(newmode).c_str());
			break;
	}
}

void SimulCam::do_restart() {
	io.msg(IO_WARN, "SimSeeing::do_restart() not implemented yet.");
}
