/*
 imgcam.h -- Dummy 'camera' with static images as source - header file
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

using namespace std;

const string imgcam_type = "imgcam";


/*!
 @brief Fake camera taking image files as input
 
 This class extends the Camera class and serves static images from disk as
 'camera' images. It is used in \ref ud_foamss "static simulation" as 'camera'
 device. ImgCamera is fairly simple and does not extend much 
 
 \section imgcamera_cfg Configuration parameters
 
 The ImgCamera class extends the Camera class configuration with the following 
 parameters:

 - imagefile: image file to use for simulation (relative to ptc->datadir)
 - noise (10.0): ImgCamera::noise
 
 \section imgcamera_netio Network IO
 - none

 */
class ImgCamera: public Camera {
private:
	double noise;												//!< Simulated noise intensity (rand() * noise + img * exposure)
	ImgData *img;												//!< ImgData utils are used to load frames
	uint16_t *frame;										//!< Source image is stored here
	
public:
	ImgCamera(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
	~ImgCamera();
	
	void update();	
	
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

#endif // HAVE_IMGCAM_H

/*!
 \page dev_cam_imgcam Image camera devices
 
 The ImgCamera class loads images from file and shows these.
 
 */
