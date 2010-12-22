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
#include "log.h"

using namespace std;

/*!
 @brief Generic device control class  
 
 This class provides basic functions for control of remote hardware. It opens
 a network connection and provides hooks for processing data and events. A
 derived class should add to this, and overload on_message and on_connected.
 A derived GUI class should register signal_connect and signal_message to
 be notified when new data is received.
 
 The DeviceCtrl class queries all commands available for any device when it 
 connects (see DeviceCtrl::on_connected) and DeviceCtrl is able to parse this
 and store all commands in a list. This will be shown on DevicePage as a
 dropdown list. This way, the GUI can support any device by simply providing
 this 'raw' control method.
 */
class DeviceCtrl {
public:
	typedef list<string> cmdlist_t;

protected:
	Protocol::Client protocol;
	Log &log;

	const string host, port, devname;
	
	bool ok;														//!< Hardware status
	string errormsg;										//!< Error message from hardware
	string lastreply;										//!< Last reply we got from FOAM
	string lastcmd;											//!< Last cmd we sent
	
	cmdlist_t devcmds;									//!< List of device commands available
	
	virtual void on_message(string line); //!< New data received from device
	virtual void on_connected(bool status); //!< Connection to device changed
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	DeviceCtrl(Log &log, const string, const string, const string);
	~DeviceCtrl();
	
	bool is_ok() const { return ok; }		//!< Return device status
	bool is_connected() { return protocol.is_connected(); } //!< Return device connection status
	string get_lastreply() const { return lastreply; } //!< Return last reply from device
	string get_errormsg() const { return errormsg; } //!< Get errormessage (if !is_ok()).
	cmdlist_t get_devcmds() { return devcmds; } //!< Get list of device commands
	
	virtual string getName() { return devname; } //!< Get device name
	virtual void send_cmd(const string &cmd); //!< Wrapper for sending messages to device

	Glib::Dispatcher signal_connect;		//!< Signalled when connection changes
	Glib::Dispatcher signal_message;		//!< Signalled when message received
	Glib::Dispatcher signal_commands;		//!< Signalled when new device commands received
};


#endif // HAVE_DEVICECTRL_H
