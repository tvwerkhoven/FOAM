/*
 cam.h -- generic camera input/output wrapper
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_CAM_H
#define HAVE_CAM_H

#include <fstream>

#include <stdint.h>

#include "types.h"
#include "config.h"
#include "io.h"
#include "devices.h"

/*!
 @brief Base camera class. This will be overloaded with the specific camera class.
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl) and Guus Sliepen (guus@sliepen.org)
 */
class Camera : public Device {
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	typedef enum {
		OFF = 0,
		SINGLE,
		RUNNING,
		ERROR
	} mode_t;
	
protected:
	void *image;									//!< Pointer to the image data (can be ringbuffer)
	void *darkim;									//!< Pointer to a sum of darkfield images
	void *flatim;									//!< Pointer to a sum of flatfield images
	void *gainim;									//!< Pointer to a (normalised) 1/(flat-dark) image
	int ndark;										//!< Number of images summed for darkfield
	int nflat;										//!< Number of images summed for flatfield
	
	double interval;
	double exposure;
	double gain;
	double offset;

	coord_t res;									//!< Camera pixel resolution
	int bpp;											//!< Camera pixel depth
	dtype_t dtype;								//!< Camera datatype

	Camera::mode_t mode;
	
	FILE *outfd;									//!< FD for storing data to disk
	
public:
	string darkfile;
	string flatfile;
	
	double get_interval() { return interval; }
	double get_exposure() { return exposure; }
	double get_gain() { return gain; }
	int get_width() { return res.x; }
	int get_height() { return res.y; }
	coord_t get_res() { return res; }
	int get_depth() { return bpp; }
	Camera::mode_t get_mode() { return mode; }
	dtype_t get_dtype() { return dtype; }
	double get_offset() { return offset; }

	void set_mode(Camera::mode_t newmode) { mode = newmode; }
	void set_interval(double value) { interval = value; }
	void set_exposure(double value) { exposure = value; }
	void set_gain(double value) { gain = value; }
	void set_offset(double value) { offset = value; }

	// From Devices::
	virtual int verify() { return 0; }
	virtual void on_message(Connection *conn, std::string line) { ; }
	virtual void on_connect(Connection *conn, bool status) { ; }

	// To be implemented in derived class
	virtual int thumbnail(Connection *) { return -1; }
	virtual int monitor(void *frame, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) { return -1; }
	virtual int store(Connection *) { return -1; }
	
	virtual ~Camera() {};
	// TODO: how to init res.x(-1), res.y(-1), ?
	Camera(Io &io, string name, string port): 
	Device(io, name, port),
	interval(1.0), exposure(1.0), gain(1.0), offset(0.0), bpp(-1), dtype(DATA_UINT16), mode(Camera::OFF), outfd(0) { ; }
};


#endif /* HAVE_CAM_H */
