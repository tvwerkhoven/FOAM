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
 @brief TODO Camera control class  
 */
class CamCtrl: public DeviceCtrl {
public:
	//Protocol::Client protocol;
	Protocol::Client monitorprotocol;

	double exposure;
	double interval;
	double gain;
	double offset;
	int32_t width;
	int32_t height;
	int32_t depth;
	std::string filename;

	volatile enum {
		OFF,
		SLAVE,
		MASTER,
	} mode;

//	bool ok;
	bool enabled;
//	std::string errormsg;

	virtual void on_message(std::string line);
	void on_monitor_message(std::string line);
	virtual void on_connected(bool connected);

	public:
	class exception: public std::runtime_error {
		public:
		exception(const std::string reason): runtime_error(reason) {}
	};

//	const std::string name;
//	const std::string host;
//	const std::string port;
	CamCtrl(const std::string name, const std::string host, const std::string port);
	~CamCtrl();

	volatile enum state {
		UNDEFINED = -2,
		ERROR = -1,
		READY = 0,
		WAITING,
		BURST,
	} state;

	double get_exposure() const;
	double get_interval() const;
	double get_gain() const;
	double get_offset() const;
	int32_t get_width() const;
	int32_t get_height() const;
	int32_t get_depth() const;
	std::string get_filename() const;
	
	double r, g, b;
	uint8_t thumbnail[32 * 32];
	struct {
		pthread::mutex mutex;
		void *image;
		size_t size;
		int x1;
		int y1;
		int x2;
		int y2;
		int scale;
		double dx;
		double dy;
		double cx;
		double cy;
		double cr;
		uint32_t *histogram;
		int depth;
	} monitor;

	void get_thumbnail();
	bool is_master() const;
	bool is_slave() const;
	bool is_off() const;
	bool is_enabled() const;

	void set_exposure(double value);
	void set_interval(double value);
	void set_gain(double value);
	void set_offset(double value);
	void set_master();
	void set_slave();
	void set_off();
	void set_enabled(bool value = true);
	void set_filename(const std::string &filename);
	void set_fits(const std::string &fits);

	void darkburst(int count);
	void flatburst(int count);
	void burst(int count, int fsel = 0);
	void grab(int x1, int y1, int x2, int y2, int scale = 1, bool df_correct = false, int fsel = 0);

	enum state get_state() const;
	bool wait_for_state(enum state desiredstate, bool condition = true);
	bool wait_for_state() {return wait_for_state(UNDEFINED, false);}
	bool connect();
//	bool is_ok() const;
//	std::string get_errormsg() const;

	Glib::Dispatcher signal_thumbnail;
	Glib::Dispatcher signal_monitor;
	//Glib::Dispatcher signal_update;
	
};

#endif // HAVE_CAMCTRL_H
