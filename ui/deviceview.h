/*
 deviceview.h -- generic device viewer
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_DEVICEVIEW_H
#define HAVE_DEVICEVIEW_H

#include <glibmm/dispatcher.h>
#include <gtkmm.h>

#include "pthread++.h"
#include "log.h"
#include "widgets.h"

#include "foamcontrol.h"
#include "devicectrl.h"
#include "protocol.h"

using namespace Gtk;
using namespace std;

/*!
 @brief Generic device viewing class  

 This is the basic device GUI. It is the GUI counterpart of DeviceCtrl, which
 controls a remote piece of hardware. It provides a basis for implementation
 of more sophisticated GUIs.
 
 The DeviceCtrl class queries all commands available for any device when it 
 connects (see DeviceCtrl::on_connected) and DeviceCtrl is able to parse this
 and store all commands in a list. This will be shown on DevicePage as a
 dropdown list. This way, the GUI can support any device by simply providing
 this 'raw' control method.
 */
class DevicePage: public Gtk::VBox {
protected:
	DeviceCtrl *devctrl;								//!< Network connection to device
	
	FoamControl &foamctrl;							//!< Reference to base connection to FOAM
	Log &log;														//!< Log message in the GUI here
	
	string devname;											//!< Device name
	
	Frame devframe;											//!< Generic device controls Frame
	HBox devhbox;												//!< Generic device controls HBox
	ComboBoxText dev_cmds;							//!< All available commands for this device
	LabeledEntry dev_val;								//!< Value or options for this command
	Button dev_send;										//!< Send command
	
	void on_dev_send_activate();				//!< Callback for dev_send and dev_val

public:
	DevicePage(DeviceCtrl *devctrl, Log &log, FoamControl &foamctrl, string n);
	virtual ~DevicePage();
	
	virtual void on_message_update();		//!< Update GUI when DeviceCtrl::signal_message is called
	virtual void on_connect_update();		//!< Update GUI when DeviceCtrl::signal_connect is called
	virtual void on_commands_update();	//!< Hooks to DeviceCtrl::signal_commands
	virtual void disable_gui();					//!< Disable GUI when disconnected
	virtual void enable_gui();					//!< Enable GUI when connected
	virtual void clear_gui();						//!< Clear GUI on init or reconnect
};

#endif // HAVE_DEVICEVIEW_H

/*!
 
 \page dev_ui Devices: DevicePage and DeviceCtrl
 
 To control a Device, there are two relevant classes. DeviceCtrl connects to 
 the Device and sends & receives commands and data, while DevicePage is a
 user interface to DeviceCtrl, and provides the user with a GUI to control
 DeviceCtrl and thus the Device itself.
 
 \section dev_devctrl DeviceCtrl
 
 DeviceCtrl connects to the Device with a Protocol::Client, and handles events
 from this connection with DeviceCtrl::on_message (connected to 
 protocol.slot_message) and DeviceCtrl::on_connected (to 
 protocol.slot_connected). DeviceCtrl itself handles simple I/O that is
 available for all devices (see relevant functions for details), but cannot
 do more than that because the specific Device is unknown. To provide more
 fine-grained control, one has to derive the DeviceCtrl class and add 
 additional logic to handle another device (i.e. a camera).
 
 \subsection dev_devctrl_der Deriving DeviceCtrl
 
 When deriving DeviceCtrl, you don't have to register protocol.slot_message 
 and protocol.slot_connected for your own class. These are already registered
 to the virtual functions on_message and on_connected by the baseclass, so the
 newly derived functions will be called automatically. You do need to call the
 base functions though, or replace all functionality yourself.
 
 \subsection dev_devctrl_sig DeviceCtrl signals
 
 There are several signals (Glib::Dispatcher) in DeviceCtrl that can be used
 to connect to when certain functionality is required. These include:
 - DeviceCtrl::signal_connect
 - DeviceCtrl::signal_message
 - DeviceCtrl::signal_commands
 each called in specific cases.
 
 \section dev_devpage DevicePage
 
 DevicePage is the interface to DeviceCtrl and provides a nice GUI to send
 the commands, so you don't have to type everything. It should work together
 with DeviceCtrl closely, which is where the Glib::Dispatchers come in handy.
 
 DevicePage handles the following signals:

 - DevicePage::on_connect_update() connects to DeviceCtrl::signal_connect() and handles (dis)connection update for the GUI
 - DevicePage::on_message_update() connects to DeviceCtrl::signal_message() and handles GUI updates
 - DevicePage::on_command_update() connects to DeviceCtrl::signal_commands() and handles (new) raw device commands
 
 Each GUI page should have several basic functions:
 - One function for each user interaction callback (i.e. pressing buttons, entering text), these *only* send commands to FOAM
 - One function for each of the events on_message and on_connect: these reflect the changes from FOAM in the GUI
 - clear_gui(), enable_gui() and disable_gui() are highly recommended to do exactly these things. Skeletons are already implemented in DevicePage.
 
 \section dev_moreinfo See also
 - \subpage dev_cam_ui "Camera device UI"
 - \subpage dev_wfs_ui "Wavefront sensor device UI"
 
 */
