/*
 fw1394cam.h -- IEEE 1394 camera module
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
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


#ifndef HAVE_FW1394CAM_H
#define HAVE_FW1394CAM_H

#include <stdint.h>

#include "config.h"
#include "io.h"

#include "camera.h"

const string FW1394cam_type = "FW1394cam";
using namespace std;

class FW1394Camera: public Camera {
private:
	// 1394-specific hardware instructions
	dc1394 dc1394;
	dc1394::camera *camera;
	
public:
	FW1394Camera(Io &io, foamctrl *ptc, string name, string port, string conffile);
	~FW1394Camera();

	// From Camera::
	void cam_handler();
	void cam_set_exposure(double value);
	double cam_get_exposure();
	void cam_set_interval(double value);
	double cam_get_interval();
	void cam_set_gain(double value);
	double cam_get_gain();
	void cam_set_offset(double value);
	double cam_set_offset();
	
	void cam_set_mode(mode_t newmode);
	void do_restart();
};

#endif // HAVE_1394CAM_H
