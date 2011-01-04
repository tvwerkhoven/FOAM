/*
 imgcam.h -- Dummy 'camera' with static images as source - header file
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

#ifndef HAVE_IMGCAM_H
#define HAVE_IMGCAM_H

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <stdint.h>

#include "config.h"
#include "io.h"
#include "imgdata.h"

#include "camera.h"

const string imgcam_type = "imgcam";

using namespace std;

/*!
 @brief Fake camera taking image files as input
 
 @todo Document this
 */
class ImgCamera: public Camera {
private:
	double noise;												//!< Simulated noise intensity
	ImgData *img;												//!< Use ImgData utils to load frames
	uint16_t *frame;										//!< Frame stored here
	
public:
	ImgCamera(Io &io, foamctrl *ptc, string name, string port, Path &conffile, bool online=true);
	~ImgCamera();
	
	void update();	
	
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

#endif // HAVE_IMGCAM_H
