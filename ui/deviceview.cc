/*
 deviceview.cc -- generic device viewer
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

#include "foamcontrol.h"
#include "protocol.h"
#include "format.h"

#include "deviceview.h"

using namespace std;
using namespace Gtk;

DevicePage::DevicePage(Log &log, FoamControl &foamctrl, string n): 
foamctrl(foamctrl), log(log), devname(n)  {
	printf("%x:DevicePage::DevicePage()\n", (int) pthread_self());
		
//	infobox.set_spacing(4);	
//	infobox.pack_start(infolabel, PACK_SHRINK);
//	infoframe.add(infobox);
	
	// Add frames to parent VBox
	set_spacing(4);
//	pack_start(infoframe, PACK_SHRINK);
	
	show_all_children();	
}

DevicePage::~DevicePage() {
	
}

int DevicePage::init() {
	// Start device controller
	devctrl = new DeviceCtrl(foamctrl.host, foamctrl.port, devname);
	
	// GUI update callback (from protocol thread to GUI thread)
	devctrl->signal_update.connect(sigc::mem_fun(*this, &DevicePage::on_message_update));
	
	// Run once for init
	on_message_update();
	
	return 0;
}

void DevicePage::on_message_update() {
	printf("%x:DevicePage::on_message_update()\n", (int) pthread_self());
	//infolabel.set_text("Device: " + devctrl->getName() + " Info: " + devctrl->getInfo());
}
