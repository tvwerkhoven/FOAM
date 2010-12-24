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
#include "log.h"

using namespace std;

CamCtrl::CamCtrl(Log &log, const string h, const string p, const string n):
	DeviceCtrl(log, h, p, n),
	monitorprotocol(host, port, devname)
{
	fprintf(stderr, "%x:CamCtrl::CamCtrl()\n", (int) pthread_self());
	
	mode = OFF;
	
	exposure = 0;
	interval = 0;
	gain = 0;
	offset = 0;
	width = 0;
	height = 0;
	depth = 0;
	mode = OFF;
	nstore = 0;
	
	monitorprotocol.slot_message = sigc::mem_fun(this, &CamCtrl::on_monitor_message);
}

CamCtrl::~CamCtrl() {
	fprintf(stderr, "%x:CamCtrl::~CamCtrl()\n", (int) pthread_self());
	set_mode(OFF);
}

void CamCtrl::connect() {
	DeviceCtrl::connect();
	monitorprotocol.connect();
}

void CamCtrl::on_connected(bool conn) {
	DeviceCtrl::on_connected(conn);
	fprintf(stderr, "%x:CamCtrl::on_connected(conn=%d)\n", (int) pthread_self(), conn);
	
	if (conn) {
		send_cmd("get mode");
		send_cmd("get exposure");
		send_cmd("get interval");
		send_cmd("get gain");
		send_cmd("get offset");
		send_cmd("get width");
		send_cmd("get height");
		send_cmd("get depth");
		send_cmd("get filename");
	}
}

void CamCtrl::on_message(string line) {
	DeviceCtrl::on_message(line);
	fprintf(stderr, "%x:CamCtrl::on_message(line=%s)\n", (int) pthread_self(), line.c_str());
	
	if (!ok) {
		return;
	}
	// Discard first 'ok' or 'err' (DeviceCtrl::on_message() already parsed this)
	string stat = popword(line);
	
	// Get command
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

	signal_message();
}

//!< @bug If this function returns, there is a problem in camview.cc
void CamCtrl::on_monitor_message(string line) {
	fprintf(stderr, "%x:CamCtrl::on_monitor_message(line=%s)\n", (int) pthread_self(), line.c_str());
	// Line has to start with 'ok image'
	if(popword(line) != "ok") {
		log.add(Log::ERROR, "image grab error (err=" + line + ")");
		return;
	}
	if(popword(line) != "image") {
		log.add(Log::ERROR, "no image from grab (err=" + line + ")");
		return;
	}

	// The rest of the line is: <size> <x1> <y1> <x2> <y2> <scale> [histogram] [avg] [rms]
	size_t size = popsize(line);
	int histosize = (1 << depth) * sizeof *monitor.histo;
	int x1 = popint(line);
	int y1 = popint(line);
	int x2 = popint(line);
	int y2 = popint(line);
	int scale = popint(line);
	bool do_histo = false;
	double avg=0, rms=0;
	int min=INT_MAX, max=0;

	string extra;
	
	// Extra options might be: histogram, avg, rms, min, max
	while(!(extra = popword(line)).empty()) {
		if(extra == "histogram") {
			do_histo = true;
		} else if(extra == "avg") {
			avg = popdouble(line);
		} else if(extra == "rms") {
			rms = popdouble(line);
		} else if(extra == "min") {
			min = popint(line);
		} else if(extra == "max") {
			max = popint(line);
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
		monitor.avg = avg;
		monitor.rms = rms;
		monitor.min = min;
		monitor.max = max;

		if(do_histo)
			monitor.histo = (uint32_t *)realloc(monitor.histo, histosize);
	}

	monitorprotocol.read(monitor.image, monitor.size);

	if(do_histo)
		monitorprotocol.read(monitor.histo, histosize);

	signal_monitor();
}

void CamCtrl::get_thumbnail() { send_cmd("thumbnail"); }
double CamCtrl::get_exposure() const { return exposure; }
double CamCtrl::get_interval() const { return interval; }
double CamCtrl::get_gain() const { return gain; }
double CamCtrl::get_offset() const { return offset; }
int32_t CamCtrl::get_width() const { return width; }
int32_t CamCtrl::get_height() const { return height; }
int32_t CamCtrl::get_depth() const { return depth; }
std::string CamCtrl::get_filename() const { return filename; }

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

void CamCtrl::set_mode(const mode_t m) { send_cmd(format("set mode %s", get_modestr(m).c_str())); }
void CamCtrl::set_exposure(double value) { send_cmd(format("set exposure %lf", value)); }
void CamCtrl::set_interval(double value) { send_cmd(format("set interval %lf", value)); }
void CamCtrl::set_gain(double value) { send_cmd(format("set gain %lf", value)); }
void CamCtrl::set_offset(double value) { send_cmd(format("set offset %lf", value)); }
void CamCtrl::set_filename(const string &filename) { send_cmd("set filename :" + filename); }
void CamCtrl::set_fits(const string &fits) { send_cmd("set fits " + fits); }

void CamCtrl::darkburst(int32_t count) {
	string command = format("dark %d", count);
	mode = UNDEFINED;
	send_cmd(command);
}

void CamCtrl::flatburst(int32_t count) {
	string command = format("flat %d", count);
	mode = UNDEFINED;
	send_cmd(command);
}

void CamCtrl::burst(int32_t count, int32_t fsel) {
	string command = format("burst %d", count);
	if(fsel > 1)
		command += format(" select %d", fsel);
	mode = UNDEFINED;
	send_cmd(command);
}

void CamCtrl::store(int nstore) { send_cmd(format("store %d", nstore)); }

void CamCtrl::grab(int x1, int y1, int x2, int y2, int scale, bool darkflat) {
	string command = format("grab %d %d %d %d %d histogram", x1, y1, x2, y2, scale);
	if(darkflat)
		command += " darkflat";
	monitorprotocol.write(command);
}
