/*
 camctrl.cc -- camera control class
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
 @file camctrl.h
 @brief Camera UI control 
 */

#ifndef HAVE_CAMCTRL_H
#define HAVE_CAMCTRL_H

#include <glibmm/dispatcher.h>
#include <string>

#include "pthread++.h"
#include "protocol.h"

#include "devicectrl.h"

/*!
 @brief Camera control class  
 
 @todo Improve docs
 */
class CamCtrl: public DeviceCtrl {
public:
	Protocol::Client monitorprotocol;		//!< Data channel for images

	// Camera settings
	double exposure;
	double interval;
	double gain;
	double offset;
	int32_t width;
	int32_t height;
	int32_t depth;
	std::string filename;
	
	int32_t nstore;

	typedef enum {
		OFF = 0,
		WAITING,
		SINGLE,
		RUNNING,
		CONFIG,
		ERROR,
		UNDEFINED
	} mode_t;
	mode_t mode;
	
	// From DeviceCtrl::
	virtual void on_message(std::string line);
	virtual void on_connected(bool connected);

	// For monitorprotocol
	void on_monitor_message(std::string line);
	void on_monitor_connected(bool connected);
	
public:
	CamCtrl(Log &log, const std::string name, const std::string host, const std::string port);
	~CamCtrl();

	double r, g, b;
	uint8_t thumbnail[32 * 32];
	// Camera frame
	struct monitor {
		monitor() {
			image = 0;
			size = 0;
			x1 = 0;
			x2 = 0;
			y1 = 0;
			y2 = 0;
			scale = 1;
			depth = 0;
			histogram = 0;
		}
		pthread::mutex mutex;
		void *image;
		size_t size;
		int x1;
		int y1;
		int x2;
		int y2;
		int scale;

		uint32_t *histogram;
		int depth;
	} monitor;
	
	// Get & set settings
	double get_exposure() const;
	double get_interval() const;
	double get_gain() const;
	double get_offset() const;
	int32_t get_width() const;
	int32_t get_height() const;
	int32_t get_depth() const;
	std::string get_filename() const;
	void get_thumbnail();
	mode_t get_mode() const { return mode; }
	std::string get_modestr(const mode_t m) const;
	std::string get_modestr() { return get_modestr(mode); }
	
	void set_exposure(double value);
	void set_interval(double value);
	void set_gain(double value);
	void set_offset(double value);
	void set_filename(const std::string &filename);
	void set_fits(const std::string &fits);
	void set_mode(const mode_t m);

	// Take images
	void darkburst(int count);
	void flatburst(int count);
	void burst(int count, int fsel = 0);
	void grab(int x1, int y1, int x2, int y2, int scale = 1, bool df_correct = false);
	void store(int nstore);
	
	bool connect();

	// Extra signals: new thumnail and new images
	Glib::Dispatcher signal_thumbnail;
	Glib::Dispatcher signal_monitor;
};

#endif // HAVE_CAMCTRL_H
