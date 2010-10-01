/*
 devices.h -- base hardware handling classes
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
/*! 
 @file devices.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief Generic device class, specific hardware controls are derived from this class.
 */

#ifndef HAVE_DEVICES_H
#define HAVE_DEVICES_H

#include <map>

#include "io.h"
#include "protocol.h"
#include "config.h"
#include "path++.h"

#include "foamctrl.h"

typedef Protocol::Server::Connection Connection;

/*!
 @brief Device class to handle devices in a generic fashion
 
 In order to accomodate all kinds of hardware devices, this baseclass 
 provides a template for all sorts of hardware (camera, wavefront sensor,
 deformable mirrors, etc). Because of its genericity this class does not 
 implement a lot itself, but rather depends on the derived classes to do
 the work. Nonetheless, some basic functions (such as network I/O) are
 provided here. All devices are stored in a DeviceManager class.
 */
class Device {
protected:
	Io &io;
	foamctrl *ptc;
	string name;												//!< Device name
	string type;												//!< Device type
	string port;												//!< Port to listen on
	Path conffile;											//!< Configuration file
	config cfg;													//!< Interpreted configuration file
	Protocol::Server netio;							//!< Network connection
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
		
	Device(Io &io, foamctrl *ptc, string n, string t, string p, Path &conf);
	virtual ~Device();
	
	virtual int verify() { return 0; }	//!< Verify the integrity of the device
	
	/*! 
	 @brief Called when the device receives a message
	 */
	virtual void on_message(Connection */*conn*/, std::string line) { 
		io.msg(IO_DEB2, "Device::on_message(line='%s')", line.c_str()); 
	}
	/*! 
	 @brief Called when something connects to this device
	 */
	virtual void on_connect(Connection */*conn*/, bool status) { 
		io.msg(IO_DEB2, "Device::on_connect(stat=%d)", (int) status); 
	}
	
	string getname() { return name; }
	string gettype() { return type; }
};

/*!
 @brief Device manager class to keep track of all devices in the system.
 
 To keep track of the different devices, this DeviceManager stores all
 in a map (devices).
 */
class DeviceManager {
private:
	Io &io;
	
	typedef map<string, Device*> device_t;
	device_t devices;
	int ndev;														//!< Number of devices in the system
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const std::string reason): runtime_error(reason) {}
	};
	
	/*! 
	 @brief Add a device to the list.
	 
	 Add a new device to the list. The name of the device (which will be used 
	 as a key in the devices map) should be unique.
	 
	 @param [in] *dev Device to be added. Should be downcasted to the base 'Device' class.
	 @return 0 on success, -1 if the device name already exists.
	 */
	int add(Device *dev);
	
	/*!
	 @brief Get a device from the list.
	 
	 @param [in] id The (unique) name of the device.
	 @return The device requested, NULL if it does not exist.
	 */
	Device* get(string id);
	
	/*!
	 @brief Delete a device from the list.
	 
	 @param [in] id The (unique) name of the device.
	 @return 0 on success, -1 if not found.
	 */
	int del(string id);
	
	/*!
	 @brief Return a list of all devices currently registered.
	 
	 @param [in] showtype Set to true to return a list of (name type) pairs, otherwise only return a list of names.
	 @return A list of names of all the devices, seperated by spaces. If showtype is true, the device types will also be returned.
	 */
	string getlist(bool showtype = true);
	
	int getcount() { return ndev; }			//!< Return the number of devices
	
	DeviceManager(Io &io);
	~DeviceManager();
};

#endif // HAVE_DEVICES_H
