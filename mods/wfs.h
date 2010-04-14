/*
 wfs.h -- a wavefront sensor abstraction class
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HAVE_WFS_H
#define HAVE_WFS_H

#include <fstream>

#include <string>
#include <stdint.h>
#include <sys/types.h>
#include "types.h"
#include "config.h"
#include "cam.h"
#include "io.h"

/*!
 @brief Base wavefront-sensor class. This will be overloaded with the specific wfs type
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 */
class Wfs {
protected:
	string conffile;
	Io &io;
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	string name;									//!< WFS name
	string camtype;								//!< Camera type/model
	string wfstype;								//!< WFS type/model
	
	Camera *cam;									//!< Camera specific class
	
	virtual int verify() = 0;			//!< Verify settings
	virtual int calibrate() = 0;	//!< Calibrate WFS
	virtual int measure() = 0;		//!< Measure abberations
	
	static Wfs *create(Io &io, config &config);	//!< Initialize new wavefront sensor
	static Wfs *create(Io &io, string conffile) {
		io.msg(IO_DEB2, "Wfs::create(conffile=%s)", conffile.c_str());
		
		ifstream fin(conffile.c_str(), ifstream::in);
		if (!fin.is_open()) throw("Could not open configuration file!");
		fin.close();
		
		config config(conffile);
		return Wfs::create(io, config);
	};	//!< Initialize new wavefront sensor
	virtual ~Wfs() {}
	Wfs(Io &io): io(io) { ; }
};

#endif // HAVE_WFS_H
