/*
 deviceview.h -- generic device viewer
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
	virtual void disable_gui();					//!< Disable GUI when disconnected @todo Should this be virtual? Could also be regular such that each class calls its own function
	virtual void enable_gui();					//!< Enable GUI when connected
	virtual void clear_gui();						//!< Clear GUI on init or reconnect
};

#endif // HAVE_DEVICEVIEW_H

/*!
 
 \page dev Devices: DevicePage and DeviceCtrl
 
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
 
 When deriving DeviceCtrl, you can again (and should) register 
 protocol.slot_message and protocol.slot_connected for your own class. These
 handles are sigc::slot's and thus can handle multiple callback functions. You 
 therefore do not need to call the base functions.
 
 \subsection devio Device I/O
 
 <ul>
 <li>DeviceCtrl handles I/O per device</li>
 <ul>
 <li>on connect: request info, DeviceCtrl::signal_connect()</li>
 <li>on disconnect: update internals, DeviceCtrl::signal_connect()</li>
 <li>on message: update internals, DeviceCtrl::signal_message()</li>
 </ul>
 <li>DevicePage handles processes signals</li>
 <ul>
 <li>DevicePage::on_connect_update() connects to DeviceCtrl::signal_connect() and handles (dis)connection update for the GUI</li>
 <li>DevicePage::on_message_update() connects to DeviceCtrl::signal_message() and handles GUI updates</li>
 </ul>
 </ul>
 
 Each GUI page should have several basic functions:
 - One function for each user interaction callback (i.e. pressing buttons, entering text), these *only* send commands to FOAM
 - One function for each of the events on_message and on_connect: these reflect the changes from FOAM in the GUI
 - DevicePage::clear_gui(), DevicePage::enable_gui() and DevicePage::disable_gui() are highly recommended to do exactly these things. Skeletons are already implemented in DevicePage.
 
 \section devctrl Devices
 
 The main aim of the GUI is to control devices running under FOAM. When 
 FoamControl connects to an instance of FOAM, it queries which devices are 
 connected to the system (see FoamControl::on_connected). This is processed
 by FoamControl::on_message and when new devices are found 
 FoamCtrl::signal_device() is triggered.
 
 When a new device is detected, the appropriate GUI class is instantiated and 
 added to the GUI. The GUI class will start a control connection to the device
 and handle I/O. The basic classes to achieve this are DevicePage and 
 DeviceCtrl. These can be overloaded to provide more detailed control over a
 device.
 
 \section moreinfo More information
 
 More information can be found on these pages:
 - \subpage dev_cam "Camera devices"
 
 */
