/*
 dummy.h -- Dummy camera modules
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


#ifndef HAVE_DUMMYCAM_H
#define HAVE_DUMMYCAM_H

#include <stdint.h>

#include "config.h"
#include "io.h"
#include "path++.h"

#include "camera.h"

const std::string dummycam_type = "dummycam";

using namespace std;

class DummyCamera: public Camera {
private:
	void update();
	double noise;
	
public:
	DummyCamera(Io &io, foamctrl *ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~DummyCamera();
	
	// From Camera::
	void cam_handler();
	void cam_set_exposure(const double value);
	double cam_get_exposure();
	void cam_set_interval(const double value);
	double cam_get_interval();
	void cam_set_gain(const double value);
	double cam_get_gain();
	void cam_set_offset(const double value);
	double cam_get_offset();

	void cam_set_mode(const mode_t newmode);
	void do_restart();
	
	// From Devices::
	int verify() { return 0; }
	void on_message(Connection *const conn, std::string line);
};

#endif // HAVE_DUMMYCAM_H
