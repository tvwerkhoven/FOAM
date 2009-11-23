/*
 wfs.h -- a wavefront sensor abstraction class

 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __WFS_H__
#define __WFS_H__

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include "types.h"
#include "config.h"

/*!
 @brief Base camera class. This will be overloaded with the specific camera class.
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl) and Guus Sliepen (guus@sliepen.org)
 */
class Camera {
	void *image;									//!< Pointer to the image data (can be ringbuffer)
	void *darkim;									//!< Pointer to a darkfield image
	void *flatim;									//!< Pointer to a flatfield image
	void *gainim;									//!< Pointer to a flat-dark image
	int ff;												//!< Number of images summed for dark- or flatfield
	
	public:

	typedef enum {
		OFF = 0,
		SLAVE = 1,
		MASTER = 2,
		RUNNING = 3,
	} mode;
	
	coord_t res;									//!< Camera pixel resolution
	int bpp;											//!< Camera pixel depth
	int dtype;										//!< Camera datatype
	
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
	virtual mode get_mode() {return OFF;}
	
	virtual void set_exposure(double) {}
	virtual void set_interval(double) {}
	virtual void set_gain(double) {}
	virtual void set_offset(double) {}
	virtual void set_mode(mode) {}
	
	virtual bool thumbnail(uint8_t *) {return false;}
	virtual void init_capture() {}
	virtual bool capture(int fd) {return false;}
	virtual bool monitor(void *frame, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) {return false;}
	
	static Camera *create(config &config);
	virtual ~Camera() {};
};

/*!
 @brief Base wavefront-sensor class. This will be overloaded with the specific wfs type
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 */
class Wfs {
	
	public:
	string name;									//!< WFS name
	
	int camtype;									//!< Camera type/model
	int wfstype;									//!< WFS type/model
	
	string wfscfg;								//!< WFS configuration file
	string camcfg;								//!< Camera configuration file
	
	Camera *cam;									//!< Camera specific class
	
	virtual int verify() { return 0; }
	
	static Wfs *create(config &config);	//!< Initialize new wavefront sensor
	virtual ~Wfs() {}
};

#endif // __WFS_H__