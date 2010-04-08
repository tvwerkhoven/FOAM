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
#include "cam.h"

/*!
 @brief Base camera class. This will be overloaded with the specific camera class.
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl) and Guus Sliepen (guus@sliepen.org)
 @data // :tim:20091126 
 */
class Camera {
protected:
	Io &io;
	
	void *image;									//!< Pointer to the image data (can be ringbuffer)
	void *darkim;									//!< Pointer to a darkfield image
	void *flatim;									//!< Pointer to a flatfield image
	void *gainim;									//!< Pointer to a flat-dark image
	int ff;												//!< Number of images summed for dark- or flatfield
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	typedef enum {
		OFF = 0,
		SLAVE = 1,
		MASTER = 2,
		RUNNING = 3,
	} mode;
	
	coord_t res;									//!< Camera pixel resolution
	int bpp;											//!< Camera pixel depth
	dtype_t dtype;								//!< Camera datatype
	
	string darkfile;
	string flatfile;
	
	virtual int verify() { return 0; }
	
	virtual double get_exposure() {return 0;}
	virtual double get_interval() {return 0;}
	virtual double get_gain() {return 0;}
	virtual double get_offset() {return 0;}
	virtual int get_width() {return 0;}
	virtual int get_height() {return 0;}
	virtual int get_depth() {return 0;}
	virtual dtype_t get_dtype() {return dtype;}
	virtual mode get_mode() {return OFF;}
	
	virtual void set_exposure(double) {}
	virtual void set_interval(double) {}
	virtual void set_gain(double) {}
	virtual void set_offset(double) {}
	virtual void set_mode(mode) {}
	
	virtual bool thumbnail(uint8_t *) {return false;}
	virtual void init_capture() {}
	virtual bool get_image(void **out) {return false;}
	virtual bool capture(int fd) {return false;}
	virtual bool monitor(void *frame, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) {return false;}
	
	static Camera *create(Io &io, config &config);
	static Camera *create(Io &io, string conffile) {
		io.msg(IO_DEB2, "Camera::create(conffile=%s)", conffile.c_str());
		
		ifstream fin(conffile.c_str(), ifstream::in);
		if (!fin.is_open()) throw("Could not open configuration file!");
		fin.close();
		
		config cfg(conffile);
		return Camera::create(io, cfg);
	}
	virtual ~Camera() {};
	Camera(Io &io): io(io) { ; }
};


#endif /* HAVE_CAM_H */
