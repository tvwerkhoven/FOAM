/*
 imgcam.h -- Dummy 'camera' with static images as source - header file
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include <sys/time.h>
#include <time.h>
#include <math.h>
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include "config.h"
#include "io.h"
#include "imgio.h"
#include "cam.h"

static const string imgcam_type = "imgcam";

using namespace std;

/*!
 @brief Fake camera taking image files as input
 
 @todo Document this
 */
class ImgCamera: public Camera {
private:
	double noise;												//!< Simulated noise intensity
	Imgio *img;													//!< Use Imgio utils to load frames
	uint16_t *frame;										//!< Frame stored here
	
public:
	ImgCamera(Io &io, string name, string port, config *config);
	~ImgCamera();
	
	void update(bool blocking);
	
	virtual int verify();
	virtual void on_message(Connection *, std::string);
//	virtual void on_connect(Connection *conn, bool status); // Not re-implemented here

	virtual int thumbnail(Connection *);	
	virtual int monitor(void *out, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale);
	virtual int store(Connection *);
};

#endif // HAVE_IMGCAM_H
