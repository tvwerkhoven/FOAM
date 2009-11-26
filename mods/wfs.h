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

#include <fstream>

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include "types.h"
#include "config.h"
#include "cam.h"

/*!
 @brief Base wavefront-sensor class. This will be overloaded with the specific wfs type
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 */
class Wfs {
	protected:
	string conffile;
	
	public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	string name;									//!< WFS name
	string camtype;								//!< Camera type/model
	string wfstype;								//!< WFS type/model
	
	Camera *cam;									//!< Camera specific class
	
	virtual int verify() { return 0; } //!< Verify settings
	virtual int calibrate(void) { return 0; } //!< Initialize WFS
	virtual void* measure() { return NULL; }
	
	static Wfs *create(config &config);	//!< Initialize new wavefront sensor
	static Wfs *create(string conffile) {
		io->msg(IO_DEB2, "Wfs::create(string conffile)");
		
		ifstream fin(conffile.c_str(), ifstream::in);
		if (!fin.is_open()) throw("Could not open configuration file!");
		fin.close();
		
		config config(conffile);
		return Wfs::create(config);
	};	//!< Initialize new wavefront sensor
	virtual ~Wfs() {}
};

#endif // __WFS_H__