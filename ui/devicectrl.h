/*
 devicectrl.h -- generic device control class
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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
#include <stdexcept>

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
	const string host, port, devname;		//!< Connection details

	Protocol::Client protocol;
	Log &log;
	
	bool ok;														//!< Hardware status
	bool calib;													//!< Calibration status
	uint32_t init;											//!< Initial setup complete?
	string errormsg;										//!< Error message from hardware
	string lastreply;										//!< Last reply we got from FOAM
	string lastcmd;											//!< Last cmd we sent
	
	cmdlist_t devcmds;									//!< List of device commands available
	set<string> cmd_ign_list;						//!< List of commands to ignore in the logging window

	/*! @brief New data received from Device over network
	 
	 This virtual function is the GUI version of Device::on_message on the 
	 hardware part of FOAM. The top function is called first, tries to parse the
	 message and if succesful, does something with the data. If the message was
	 not understood, it passes the data on to the base function, until it is 
	 parsed. If finally DeviceCtrl::on_message() is called (this function),
	 error handling may occur if the message is not understood.
	 */
	virtual void on_message(string line);
	/*! @brief Common functions for on_message()
	 
	 This function is called first when new data is received. It handles common
	 tasks and then delegates the rest to on_message().
	 */
	void on_message_common(string line);
	virtual void on_connected(bool status); //!< Connection to device changed
	
public:
	//! @todo This should go somewhere more general
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	DeviceCtrl(Log &log, const string, const string, const string);
	~DeviceCtrl();
	virtual void connect();							//!< Connect to FOAM. Does not happen immediately because we wait for the GUI to be ready (see DeviceView::)
	
	bool is_ok() const { return ok; }		//!< Return device status
	bool is_calib() const { return calib; } //!< Return device calibration status
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
