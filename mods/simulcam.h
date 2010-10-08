/*
 simulcam.h -- atmosphere/telescope simulator camera header
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


#ifndef HAVE_SIMULCAM_H
#define HAVE_SIMULCAM_H

#include <stdint.h>

#include "config.h"
#include "io.h"
#include "simseeing.h"
#include "camera.h"

using namespace std;

const string SimulCam_type = "SimulCam";

class SimulCam: public Camera {
private:
	SimSeeing seeing;			//!< This class simulates the atmosphere, telescope and lenslet array
	SimWfs shwfs;					//!< This class simulates the atmosphere, telescope and lenslet array
public:
	SimulCam(Io &io, foamctrl *ptc, string name, string port, Path &conffile);
	~SimulCam();
	
	// From Camera::
	void cam_handler();
	void cam_set_exposure(double value);
	double cam_get_exposure();
	void cam_set_interval(double value);
	double cam_get_interval();
	void cam_set_gain(double value);
	double cam_get_gain();
	void cam_set_offset(double value);
	double cam_get_offset();
	
	void cam_set_mode(mode_t newmode);
	void do_restart();
};

#endif // HAVE_SIMULCAM_H
