/*
 dummy.h -- Dummy camera modules
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


#ifndef HAVE_DUMMYCAM_H
#define HAVE_DUMMYCAM_H

#include <stdint.h>

#include "config.h"
#include "io.h"

#include "camera.h"

using namespace std;

class DummyCamera: public Camera {
private:
	void update(bool blocking);
	double noise;
	
public:
	DummyCamera(Io &io, foamctrl *ptc, string name, string port, string conffile);
	~DummyCamera();
	
	// From Camera::
	virtual void cam_handler();		//!< Camera handler
	virtual void *cam_queue(void *data, void *image, struct timeval *tv = 0); //!< Store frame in buffer, returns oldest frame if buffer is full
	virtual void on_message(Connection* /* conn */, std::string /* line */) { } ;
	
	// Get thumbnail
	bool thumbnail(uint8_t *out);
	bool monitor(void *out, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale);
	bool capture();
	
};

#endif // HAVE_DUMMYCAM_H
