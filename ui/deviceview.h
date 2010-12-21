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
	DevicePage(Log &log, FoamControl &foamctrl, string n, bool is_parent=false);
	virtual ~DevicePage();
	
	virtual void init();
	virtual void on_message_update();		//!< Update GUI when device reports state changes
	virtual void on_connect_update();		//!< Update GUI when connected or disconnected
	
	virtual void disable_gui();					//!< Disable GUI when disconnected
	virtual void enable_gui();					//!< Enable GUI when connected
	virtual void clear_gui();						//!< Clear GUI on init or reconnect
};

#endif // HAVE_DEVICEVIEW_H
