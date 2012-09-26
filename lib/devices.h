/*
 devices.h -- base hardware handling classes
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


#ifndef HAVE_DEVICES_H
#define HAVE_DEVICES_H

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <vector>
#include <stdexcept>

#include "io.h"
#include "protocol.h"
#include "config.h"
#include "path++.h"

#include "foamctrl.h"

using namespace std;

typedef Protocol::Server::Connection Connection;

/*!
 @brief Device class to handle devices in a generic fashion
 
 In order to accomodate all kinds of hardware devices, this base class 
 provides a template for all sorts of hardware (camera, wavefront sensor,
 deformable mirrors, etc). Because of its generic nature this class does not 
 implement a lot itself, but rather depends on the derived classes to do
 the work. Nonetheless, some basic functions (such as network I/O) are
 provided here. All devices are stored in a DeviceManager object.
 
 One thing that is supported by the Device class is a list of commands 
 supported by this piece of hardware. Each time this class is (sub)derived,
 this should be updated. These commands are stored in Device::cmd_list and 
 things can be added with add_cmd().
 
 @todo Add signal for measurement complete
 */
namespace foam {

class Device {
private:
	// These should be accessed through get/set functions to notify the GUI
	bool is_calib;											//!< Device calibrated and ready for use
	bool is_ok;													//!< Device status OK & operational

	Path outputdir;											//!< Output directory for this device in case of multiple data files. This is always a subdir of ptc->datadir.

protected:
	Io &io;
	foamctrl *const ptc;
	
	const string name;									//!< Device name (must be unique)
	const string type;									//!< Device type (hierarchical, like dev.cam.simcam)
	const string port;									//!< Port to listen on
	
	vector<string> cmd_list;						//!< All commands this device supports
	void add_cmd(const string cmd) { cmd_list.push_back(cmd); } //!< Add command to list
	
	const Path conffile;								//!< Configuration file
	config cfg;													//!< Interpreted configuration file
	
	Protocol::Server *netio;						//!< Network connection with device prefix. This is multiplexed over the connection in foamctrl::
	bool online;												//!< Online flag, indicates whether this Device listens to network commands or not.
	
	
	void set_status(bool newstat);			//!< Set Device::is_ok
	bool get_status() { return is_ok; }	//!< Get Device::is_ok
	void set_calib(bool newcalib);			//!< Set Device::is_calib
	bool get_calib() { return is_calib; } //!< Get Device::is_calib

	//! @todo Might not be necessary?
	bool init();												//!< Initialisation (common for all constructors)
	
	/*! @brief Set variable, helper function for on_message
	 
	 N.B. Value should be castable to double, such that it can be formatted with %lf.
	 
	 @param [in] *conn Client connection
	 @param [in] varname Variable name
	 @param [in] value Value for this variable
	 @param [in] *var Pointer to variable
	 @param [in] min Minimum allowed value
	 @param [in] max Maximum allowed value
	 @param [in] errmsg Error message to send to Client if value is outside [min, max]
	 */
	template <class T> T set_var(Connection * const conn, const string varname, const T value, T* var, const T min=0, const T max=0, const string errmsg="") const {
		if (conn)
			conn->addtag(varname);
		
		if (errmsg != "" && (value > max || value < min)) {
			if (conn)
				conn->write("error " + varname + " :" + errmsg);
				return *var;
		}
		else {
			*var = value;
			//! @todo Check best format for reply, should be %le or %g?
			net_broadcast(format("ok %s %g", varname.c_str(), (double) *var), varname);
			return *var;
		}
	}

	/*! @brief Get variable, helper function for on_message
	 
	 @param [in] *conn Client connection
	 @param [in] varname Variable name
	 @param [in] value Value for this variable
	 @param [in] comment Comment to send along to Client (optional)
	 */
	void get_var(Connection * const conn, const string varname, const double value, const string comment="") const;

	/*! @brief Get variable, helper function for on_message -- free-form version
	 
	 @param [in] *conn Client connection
	 @param [in] varname Variable name
	 @param [in] response Response for client
	 */
	void get_var(Connection * const conn, const string varname, const string response) const;
	
	/*! 
	 @brief Callback for incoming network message
	 
	 This virtual function is called when the Device receives data over the 
	 network (i.e. commands etc.). The derived class parses this message first,
	 and if it did not understand the command it will be passed down to the base
	 class until it is known. If the base class (Device) does still not know
	 this command, it is treated as 'unknown' and replies with an error.
	 */
	virtual void on_message(Connection * const conn, string line);
	
	/*! 
	 @brief Common callback for incoming network message
	 
	 on_message_common() is called first when a message is received, followed by 
	 a series of on_message() calls.
	 */
	void on_message_common(Connection * const conn, string line);
	
	/*! 
	 @brief Called when something connects to this device
	 */
	virtual void on_connect(const Connection * const /*conn*/, const bool status) const { 
		io.msg(IO_DEB2, "Device::on_connect(stat=%d)", (int) status); 
	}
	
	/*!
	 @brief Broadcast a message over netio
	 */
	void net_broadcast(const string &msg) const {
		if (netio)
			netio->broadcast(msg);
	}

	/*!
	 @brief Broadcast a tagged message over netio
	 */
	void net_broadcast(const string &msg, const string &tag) const {
		if (netio)
			netio->broadcast(msg, tag);
	}

public:
	//! @todo This should go somewhere more general
	class exception: public std::runtime_error {
	public:
		exception(const string reason): runtime_error(reason) {}
	};
	
	Device(Io &io, foamctrl *const ptc, const string n, const string t, const string p, const Path conf=string(""), const bool online=true);
	virtual ~Device();
	
	//! @todo obsolete, can go away (no use now)
	virtual int verify() { return 0; }				//!< Verify the integrity of the device
	
	Path mkfname(const string identifier) const { return ptc->outdir + Path(type + "." + name + "_" + identifier); } //!< Make filename for single data file output
	int set_outputdir(const string identifier); //!< Set output directory for multiple datafiles (e.g. camera output)
	Path get_outputdir() const { return outputdir; } //!< Get output directory
	
	bool isonline() const { return online; }	//!< Check if this Device is online
	string getname() const { return name; }		//!< Give name of this Device
	string gettype() const { return type; }		//!< Give type of this Device
};
} // namespace foam

/*!
 @brief Device manager class to keep track of all devices in the system.
 
 To keep track of the different devices, this DeviceManager stores all
 in a map (devices).
 */
namespace foam {
class DeviceManager {
private:
	Io &io;
	
	typedef map<string, Device*> device_t;
	device_t devices;										//!< Simple list of devices, stored by name.
	
public:
	class exception: public std::runtime_error {
	public:
		exception(const string reason): runtime_error(reason) {}
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
	 @brief Return list of devices: <ndev> <devname> [devtype] {<devname> [devtype]} ...
	 
	 @param [in] showtype Set to true to return a list of (name type) pairs, otherwise only return a list of names.
	 @param [in] showonline Only show devices that are network-aware (i.e. are listening on the network)
	 @return A list of names of all the devices, seperated by spaces, prefixed by total count. If showtype is true, the device types will also be returned.
	 */
	string getlist(bool showtype = true, bool showonline=true);
	
	size_t getcount() { return devices.size(); }			//!< Return the number of devices
	
	DeviceManager(Io &io);
	~DeviceManager();
};
} // namespace foam

#endif // HAVE_DEVICES_H

/*!
 \page devmngr DeviceManager
 
 DeviceManager keeps track of which devices are connected to the system. The 
 following methods are available:
 - int DeviceManager::add(Device*) add device
 - int DeviceManager::del(string) to manage the list. 
 - Device* DeviceManager::get(string id) to get a device named 'id'
 - int DeviceManager::getcount() get number of devices
 - string DeviceManager::getlist(bool, bool) get list of devices
 
 \section devmngr_related See also
 - \ref dev "Devices info"

*/

/*!
 \page dev Devices

 In order to accomodate all kinds of hardware devices, the Device baseclass 
 provides a template for all sorts of hardware (camera, wavefront sensor,
 deformable mirrors, etc). Because of its genericity this class does not 
 implement a lot itself, but rather depends on the derived classes to do
 the work. Nonetheless, some basic functions (such as network I/O) are
 provided here. All devices are stored in a DeviceManager class.

 \section dev_derive Deriving a Device
 
 To derive a Device class, one can use the following functions for overloading:
 
 - Device::on_message(Connection * const, string)
 - Device::on_connect(const Connection * const, const bool) const
 - Device::verify()
 
 but this is only necessary if one wants to extend the functionality. If you 
 *do* overload these functions, the base functions also have to be called.
 
 \section dev_der Derived classes
 - \subpage dev_cam "Camera device"
 - \subpage dev_wfs "Wavefront sensor device"
 - \subpage dev_wfc "Wavefront corrector device"
 - \subpage dev_telescope "Telescope control device"

 \section dev_related See also
 - \ref devmngr "Device Manager info"

*/
