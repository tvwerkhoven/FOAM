/*
 dummy.h -- Dummy camera modules
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
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

using namespace std;
const string dummycam_type = "dummycam";

/*! @brief Simplest Camera implementation
 
 DummyCamera provides a simple Camera implementation which can be used for
 testing. The 'images' it generates are a sine pattern overlayed with random
 noise.
 
 \section dummycamera_cfg Configuration
 
 - noise: see DummyCamera::noise

 \section dummycamera_netio Network IO
 
 - hello world: connectivity test, should return 'ok :hello world back!'
 
 */
class DummyCamera: public Camera {
private:
	void update();
	double noise;                           //! Amplitude of the noise added to the image
	
public:
	DummyCamera(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online=true);
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
	void on_message(Connection *const conn, string line);
};

#endif // HAVE_DUMMYCAM_H

/*!
 \page dev_cam_dummy Dummy camera devices
 
 The DummyCamera class is a dummy camera and is for testing only.
 
 */