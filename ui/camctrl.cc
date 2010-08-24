/*
 camctrl.h -- camera control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2010 Guus Sliepen
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

/*!
 @file camctrl.cc
 @brief Camera UI control 
 */

#include <iostream>
#include <arpa/inet.h>
#include <string>

#include "format.h"
#include "protocol.h"
#include "devicectrl.h"
#include "camctrl.h"

using namespace std;

CamCtrl::CamCtrl(const string h, const string p, const string n): 
	DeviceCtrl(h, p, n),
	monitorprotocol(h, p, n) 
{
	fprintf(stderr, "CamCtrl::CamCtrl()\n");
	
	ok = false;
	state = UNDEFINED;
	errormsg = "Not connected";
	
	exposure = 0;
	interval = 0;
	gain = 0;
	offset = 0;
	width = 0;
	height = 0;
	depth = 0;
	mode = OFF;
	r = 1;
	g = 1;
	b = 1;

	monitor.x1 = 0;
	monitor.y1 = 0;
	monitor.x2 = 0;
	monitor.y2 = 0;
	monitor.scale = 1;
	monitor.image = 0;
	monitor.size = 0;
	monitor.histogram = 0;

//	protocol.slot_message = sigc::mem_fun(this, &CamCtrl::on_message);
//	protocol.slot_connected = sigc::mem_fun(this, &CamCtrl::on_connected);
//	protocol.connect();
	monitorprotocol.slot_message = sigc::mem_fun(this, &CamCtrl::on_monitor_message);
	monitorprotocol.connect();
}

CamCtrl::~CamCtrl() {
	set_off();
}

void CamCtrl::on_connected(bool connected) {
	
	DeviceCtrl::on_connected(connected);
	
	fprintf(stderr, "CamCtrl::on_connected()\n");
	if(!connected) {
		ok = false;
		errormsg = "Not connected";
		signal_update();
		return;
	}

	protocol.write("get exposure");
	protocol.write("get interval");
	protocol.write("get gain");
	protocol.write("get offset");
	protocol.write("get width");
	protocol.write("get height");
	protocol.write("get depth");
	protocol.write("get mode");
	protocol.write("get filename");
	protocol.write("get state");
}

void CamCtrl::on_message(string line) {
	DeviceCtrl::on_message(line);
	
	if(!ok) {
		state = ERROR;
		return;
	}

	string what = popword(line);

	if(what == "exposure")
		exposure = popdouble(line);
	else if(what == "interval")
		interval = popdouble(line);
	else if(what == "gain")
		gain = popdouble(line);
	else if(what == "offset")
		offset = popdouble(line);
	else if(what == "width")
		width = popint32(line);
	else if(what == "height")
		height = popint32(line);
	else if(what == "depth")
		depth = popint32(line);
	else if(what == "filename")
		filename = popword(line);
	else if(what == "state") {
		string statename = popword(line);
		if(statename == "ready")
			state = READY;
		else if(statename == "waiting")
			state = WAITING;
		else if(statename == "burst")
			state = BURST;
		else if(statename == "error") {
			state = ERROR;
			ok = false;
		} else {
			state = UNDEFINED;
			ok = false;
			errormsg = "Unexpected state '" + statename + "'";
		}
	} else if(what == "mode") {
		string modename = popword(line);
		if(modename == "master")
			mode = MASTER;
		else if(modename == "slave")
			mode = SLAVE;
		else
			mode = OFF;
	} else if(what == "thumbnail") {
		protocol.read(thumbnail, sizeof thumbnail);
		signal_thumbnail();
		return;
	} else if(what == "fits") {
	} else {
		//ok = false;
		//errormsg = "Unexpected response '" + what + "'";
	}

	signal_update();
}

void CamCtrl::on_monitor_message(string line) {
	if(popword(line) != "OK")
		return;
	
	if(popword(line) != "image")
		return;

	size_t size = popsize(line);
	int histosize = (1 << depth) * sizeof *monitor.histogram;
	int x1 = popint(line);
	int y1 = popint(line);
	int x2 = popint(line);
	int y2 = popint(line);
	int scale = popint(line);
	double dx = 0.0/0.0;
	double dy = 0.0/0.0;
	double cx = 0.0/0.0;
	double cy = 0.0/0.0;
	double cr = 0.0/0.0;
	bool do_histogram = false;

	string extra;

	while(!(extra = popword(line)).empty()) {
		if(extra == "histogram") {
			do_histogram = true;
		} else if(extra == "tiptilt") {
			dx = popdouble(line);
			dy = popdouble(line);
		} else if(extra == "com") {
			cx = popdouble(line);
			cy = popdouble(line);
			cr = popdouble(line);
		}
	}

	{
		pthread::mutexholder h(&monitor.mutex);
		if(size > monitor.size)
			monitor.image = (uint16_t *)realloc(monitor.image, size);
		monitor.size = size;
		monitor.x1 = x1;
		monitor.y1 = y1;
		monitor.x2 = x2;
		monitor.y2 = y2;
		monitor.scale = scale;
		monitor.dx = dx;
		monitor.dy = dy;
		monitor.cx = cx;
		monitor.cy = cy;
		monitor.cr = cr;
		monitor.depth = depth;

		if(do_histogram)
			monitor.histogram = (uint32_t *)realloc(monitor.histogram, histosize);
	}

	monitorprotocol.read(monitor.image, monitor.size);

	if(do_histogram)
		monitorprotocol.read(monitor.histogram, histosize);

	signal_monitor();
}

void CamCtrl::get_thumbnail() {
	protocol.write("thumbnail");
}

double CamCtrl::get_exposure() const {
	return exposure;
}

double CamCtrl::get_interval() const {
	return interval;
}

double CamCtrl::get_gain() const {
	return gain;
}

double CamCtrl::get_offset() const {
	return offset;
}

int32_t CamCtrl::get_width() const {
	return width;
}

int32_t CamCtrl::get_height() const {
	return height;
}

int32_t CamCtrl::get_depth() const {
	return depth;
}

std::string CamCtrl::get_filename() const {
	return filename;
}

bool CamCtrl::is_master() const {
	return mode == MASTER;
}

bool CamCtrl::is_slave() const {
	return mode == SLAVE;
}

bool CamCtrl::is_off() const {
	return mode == OFF;
}

bool CamCtrl::is_enabled() const {
	return enabled;
}

void CamCtrl::set_exposure(double value) {
	protocol.write(format("set exposure %lf", value));
}

void CamCtrl::set_interval(double value) {
	protocol.write(format("set interval %lf", value));
}

void CamCtrl::set_gain(double value) {
	protocol.write(format("set gain %lf", value));
}

void CamCtrl::set_offset(double value) {
	protocol.write(format("set offset %lf", value));
}

void CamCtrl::set_master() {
	protocol.write("set mode master");
}

void CamCtrl::set_slave() {
	protocol.write("set mode slave");
}

void CamCtrl::set_off() {
	protocol.write("set mode off");
}

void CamCtrl::set_enabled(bool value) {
	enabled = value;
}

void CamCtrl::set_filename(const string &filename) {
	protocol.write("set filename :" + filename);
}

void CamCtrl::set_fits(const string &fits) {
	protocol.write("set fits " + fits);
}

void CamCtrl::darkburst(int32_t count) {
	string command = format("dark %d", count);
	state = UNDEFINED;
	protocol.write(command);
}

void CamCtrl::flatburst(int32_t count) {
	string command = format("flat %d", count);
	state = UNDEFINED;
	protocol.write(command);
}

void CamCtrl::burst(int32_t count, int32_t fsel) {
	string command = format("burst %d", count);
	if(fsel > 1)
		command += format(" select %d", fsel);
	state = UNDEFINED;
	protocol.write(command);
}

enum CamCtrl::state CamCtrl::get_state() const {
	return state;
}

bool CamCtrl::wait_for_state(enum state desiredstate, bool condition) {
	while((ok && state != ERROR) && (state == UNDEFINED || (state == desiredstate) != condition))
		usleep(100000);

	return (state == desiredstate) == condition;
}

//bool CamCtrl::is_ok() const {
//	return ok;
//}
//
//string CamCtrl::get_errormsg() const {
//	return errormsg;
//}

void CamCtrl::grab(int x1, int y1, int x2, int y2, int scale, bool darkflat, int fsel) {
	string command = format("grab %d %d %d %d %d histogram", x1, y1, x2, y2, scale);
	if(darkflat)
		command += " darkflat";
	if(fsel)
		command += format(" select %d", fsel);
	monitorprotocol.write(command);
}
