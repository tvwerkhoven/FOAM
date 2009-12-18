/* foamcontrol.h - FOAM control connection
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
/*!
 @file foamcontrol.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM control connection class
 */

#ifndef __HAVE_FOAMCONTROL_H__
#define __HAVE_FOAMCONTROL_H__

#include "protocol.h"
#include "pthread++.h"
#include "types.h"
#include <glibmm/dispatcher.h>

class FoamControl {
	Protocol::Client protocol;
	Protocol::Client dataprotocol;
	
	aomode_t mode;
	int numwfs;
	int numwfc;
	
	bool ok;
	std::string errormsg;
	
	void on_message(std::string line);
	void on_data_message(std::string line);
	void on_connected(bool connected);
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	const std::string name;
	const std::string host;
	const std::string port;
	FoamControl(const std::string &name, const std::string &host, const std::string &port);
	~FoamControl();
	
	int get_numwfs();
	int get_numwfc();
	aomode_t get_mode() { return mode; }
	
	void set_mode(aomode_t mode);
	void shutdown() { protocol.write("shutdown"); }
	
	bool is_ok() { return ok; }
	std::string get_errormsg() { return errormsg; }
	
	Glib::Dispatcher signal_update;
};

#endif
