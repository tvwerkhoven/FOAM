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

DevicePage::DevicePage(Log &log, FoamControl &foamctrl, string devname): 
foamctrl(foamctrl), log(log), name(devname), devinfo("undef")  {
	printf("%x:DevicePage::DevicePage()\n", (int) pthread_self());
		
	infobox.set_spacing(4);	
	infobox.pack_start(infolabel, PACK_SHRINK);
	devframe.add(infobox);
	
	// Add frames to parent VBox
	set_spacing(4);
	pack_start(devframe, PACK_SHRINK);
	
	on_message_update();

	show_all_children();

	// Open control connection
	protocol.slot_message = sigc::mem_fun(this, &DevicePage::on_message);
	protocol.slot_connected = sigc::mem_fun(this, &DevicePage::on_connect);
	printf("%x:DevicePage::DevicePage(): connecting to %s:%s@%s\n", (int) pthread_self(), foamctrl.host.c_str(), foamctrl.port.c_str(), name.c_str());
	protocol.connect(foamctrl.host, foamctrl.port, name);
	
	// GUI update callback
	signal_message.connect(sigc::mem_fun(*this, &DevicePage::on_message_update));
}

DevicePage::~DevicePage() {
	
}

void DevicePage::on_message_update() {
	printf("%x:DevicePage::on_message_update()\n", (int) pthread_self());
	infolabel.set_text("Device: " + name + " Info: " + devinfo);
}

void DevicePage::on_message(string line) {
	printf("%x:DevicePage::on_message(line=%s)\n", (int) pthread_self(), line.c_str());	
	
	if (popword(line) != "ok") {
		signal_message();
		return;
	}
	
	string what = popword(line);

	if (what == "info") {
		devinfo = line;
	}
	
	signal_message();
}

void DevicePage::on_connect(bool status) {
	printf("%x:DevicePage::on_message(status=%d)\n", (int) pthread_self(), status);	
	if (status)
		protocol.write("info");

}
