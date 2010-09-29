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
	mode = UNDEFINED;
	errormsg = "Not connected";
	
	exposure = 0;
	interval = 0;
	gain = 0;
	offset = 0;
	width = 0;
	height = 0;
	depth = 0;
	mode = OFF;
	nstore = 0;
	
	r = 1;
	g = 1;
	b = 1;

	protocol.slot_message = sigc::mem_fun(this, &CamCtrl::on_message);
	monitorprotocol.slot_message = sigc::mem_fun(this, &CamCtrl::on_monitor_message);
	monitorprotocol.connect();
}

CamCtrl::~CamCtrl() {
	fprintf(stderr, "CamCtrl::~CamCtrl()\n");
	set_mode(OFF);
}

void CamCtrl::on_connected(bool connected) {
	fprintf(stderr, "CamCtrl::on_connected()\n");
	DeviceCtrl::on_connected(connected);
	
	protocol.write("get mode");
	protocol.write("get exposure");
	protocol.write("get interval");
	protocol.write("get gain");
	protocol.write("get offset");
	protocol.write("get width");
	protocol.write("get height");
	protocol.write("get depth");
	protocol.write("get filename");
}

void CamCtrl::on_message(string line) {
	fprintf(stderr, "CamCtrl::on_message()\n");
	DeviceCtrl::on_message(line);
	
	
	if(!ok) {
		mode = ERROR;
		return;
	}
	popword(line);

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
	else if(what == "store")
		nstore = popint32(line);
	else if(what == "filename")
		filename = popword(line);
	else if(what == "mode") {
		string m = popword(line);
		
		if (m == "OFF") mode = OFF;
		else if (m == "WAITING") mode = WAITING;
		else if (m == "SINGLE") mode = SINGLE;
		else if (m == "RUNNING") mode = RUNNING;
		else if (m == "CONFIG") mode = CONFIG;
		else if (m == "ERROR") {
			mode = ERROR;
			ok = false;
		}
		else {
			mode = UNDEFINED;
			ok = false;
			errormsg = "Unexpected mode '" + m + "'";
		}	
	} else if(what == "thumbnail") {
		protocol.read(thumbnail, sizeof thumbnail);
		signal_thumbnail();
		return;
	} else {
		ok = false;
		errormsg = "Unexpected response '" + what + "'";
	}

	signal_update();
}

void CamCtrl::on_monitor_message(string line) {
	if(popword(line) != "ok")
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
	bool do_histogram = false;

	string extra;

	while(!(extra = popword(line)).empty()) {
		if(extra == "histogram") {
			do_histogram = true;
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

std::string CamCtrl::get_modestr(const mode_t m) const {
	if (m == OFF) return "OFF";
	if (m == WAITING) return "WAITING";
	if (m == SINGLE) return "SINGLE";
	if (m == RUNNING) return "RUNNING";
	if (m == CONFIG) return "CONFIG";
	if (m == ERROR) return "ERROR";	
	else 
		return "UNDEFINED";
}

void CamCtrl::set_mode(const mode_t m) {
	protocol.write(format("set mode %s", get_modestr(m).c_str()));
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

void CamCtrl::set_filename(const string &filename) {
	protocol.write("set filename :" + filename);
}

void CamCtrl::set_fits(const string &fits) {
	protocol.write("set fits " + fits);
}

void CamCtrl::darkburst(int32_t count) {
	string command = format("dark %d", count);
	mode = UNDEFINED;
	protocol.write(command);
}

void CamCtrl::flatburst(int32_t count) {
	string command = format("flat %d", count);
	mode = UNDEFINED;
	protocol.write(command);
}

void CamCtrl::burst(int32_t count, int32_t fsel) {
	string command = format("burst %d", count);
	if(fsel > 1)
		command += format(" select %d", fsel);
	mode = UNDEFINED;
	protocol.write(command);
}

//enum CamCtrl::state CamCtrl::get_state() const {
//	return state;
//}
//
//
//bool CamCtrl::wait_for_state(enum state desiredstate, bool condition) {
//	while((ok && state != ERROR) && (state == UNDEFINED || (state == desiredstate) != condition))
//		usleep(100000);
//
//	return (state == desiredstate) == condition;
//}

void CamCtrl::store(int nstore) {
	protocol.write(format("store %d", nstore));
}

void CamCtrl::grab(int x1, int y1, int x2, int y2, int scale, bool darkflat) {
	string command = format("grab %d %d %d %d %d histogram", x1, y1, x2, y2, scale);
	if(darkflat)
		command += " darkflat";
	monitorprotocol.write(command);
}
