/*
 andor.h -- Andor iXON camera modules
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

#ifndef HAVE_ANDOR_H
#define HAVE_ANDOR_H

#include <stdint.h>

#include "config.h"
#include "io.h"
#include "path++.h"

#include "camera.h"

using namespace std;
const string andor_type = "andorcam";

/*!
 @brief Andor iXON camera class
 
 AndorCam is derived from Camera. It can control an Andor iXON camera.
 
 Configuration parameters:
 - cooltemp: default requested cooling temperature
 */
class AndorCam: public Camera {
private:
	
	int cooltemp;												//!< Andor requested cooling temperature
	int coolrange[2];										//!< Andor cooling temperature range
	bool cooler_on;											//!< Cooler status
	
	int cam_set_cooltemp(const int value); //!< Set requested cooling temperature
	void cam_get_coolrange();						//!< Get cooling temperature range
	
public:
	AndorCam(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~AndorCam();
	
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
	void on_message(Connection *const conn, string line);
};

#endif // HAVE_ANDOR_H

/*!
 \page dev_cam_andor Andor camera devices
 
 The AndorCam class can drive an Andor iXON camera.
 
 */