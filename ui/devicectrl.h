/*
 devicectrl.h -- generic device control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_DEVICECTRL_H
#define HAVE_DEVICECTRL_H

#include <glibmm/dispatcher.h>

#include "pthread++.h"
#include "protocol.h"

using namespace std;

/*!
 @brief Generic device control class  
 
 @todo Document this class
 */
class DeviceCtrl {
protected:
	Protocol::Client protocol;
	
	const string host, port, devname;
	string devinfo;
	
	virtual void on_message(string line);
	virtual void on_connected(bool status);
	
public:
	Glib::Dispatcher signal_update;
	
	bool ok;
	string errormsg;

	DeviceCtrl(const string, const string, const string);
	~DeviceCtrl() { ; }
	
	bool is_ok() const { return ok; }
	string get_errormsg() const { return errormsg; }
	
	virtual string getInfo() { return devinfo; }
	virtual string getName() { return devname; }
};


#endif // HAVE_DEVICECTRL_H
