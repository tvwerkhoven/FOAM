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

class DevicePage: public Gtk::VBox {
protected:
	// TODO: how to re-use this variable in derived classes? Would like to access it as ImgCam device
	// 20100823 SOLVED: when creating a new connection, rename it and then store
	//									a downcasted version to this variable, i.e.
	//									camctrl = new CamCtrl(); devctrl = (DeviceCtrl *) camctrl;
	DeviceCtrl *devctrl;								//!< Network connection to device
	
	FoamControl &foamctrl;
	Log &log;
	
	string devname;											//!< Device name
	
	// GTK stuff
	Frame infoframe;
	HBox infobox;
	Label infolabel;

public:
	DevicePage(Log &log, FoamControl &foamctrl, string n);
	virtual ~DevicePage();
	
	virtual int init();
	virtual void on_message_update();
};

#endif // HAVE_DEVICEVIEW_H