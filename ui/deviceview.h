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
#include "protocol.h"

using namespace Gtk;
using namespace std;

class DevicePage: public Gtk::VBox {
public:
	FoamControl &foamctrl;
	Log &log;
	Protocol::Client protocol;
	Glib::Dispatcher signal_message;
	string name;												//!< Device name
	string devinfo;											//!< Device info
	
	Frame devframe;
	HBox infobox;
	Label infolabel;
	
//public:
	DevicePage(Log &log, FoamControl &foamctrl, string devname);
	~DevicePage();
	
	void on_message_update();
	void on_message(string line);
	void on_connect(bool status);
};

#endif // HAVE_DEVICEVIEW_H