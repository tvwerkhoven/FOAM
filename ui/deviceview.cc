/*
 deviceview.cc -- generic device viewer
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

#include "foamcontrol.h"
#include "protocol.h"
#include "format.h"

#include "devicectrl.h"
#include "deviceview.h"

using namespace std;
using namespace Gtk;

DevicePage::DevicePage(DeviceCtrl *devctrl, Log &log, FoamControl &foamctrl, string n): 
devctrl(devctrl), foamctrl(foamctrl), log(log), devname(n),
devframe("Raw device control"), dev_val("value:"), dev_send("Send"), dev_stat("Status")
{
	log.term(format("%s", __PRETTY_FUNCTION__));
		
	clear_gui();
	disable_gui();
	
	dev_send.signal_clicked().connect(sigc::mem_fun(*this, &DevicePage::on_dev_send_activate));
	dev_val.entry.signal_activate().connect(sigc::mem_fun(*this, &DevicePage::on_dev_send_activate));

	dev_val.set_width_chars(12);
	dev_stat.set_editable(false);
	dev_stat.set_width_chars(24);

	// Init default values for extra_win
	extra_win.set_title("FOAM " + devname);
	extra_win.set_default_size(640, 200);
	extra_win.set_gravity(Gdk::GRAVITY_STATIC);
	
	devhbox.set_spacing(4);
	devhbox.pack_start(dev_cmds, PACK_SHRINK);
	devhbox.pack_start(dev_val, PACK_SHRINK);
	devhbox.pack_start(dev_send, PACK_SHRINK);
	devhbox.pack_start(dev_stat, PACK_SHRINK);
	devframe.add(devhbox);
	
	pack_start(devframe, PACK_SHRINK);
	
	show_all_children();
	
	// Setup callbacks. These functions are virtual, so always call the parent functions in case of derived classes.
	devctrl->signal_message.connect(sigc::mem_fun(*this, &DevicePage::on_message_update));
	devctrl->signal_connect.connect(sigc::mem_fun(*this, &DevicePage::on_connect_update));
	devctrl->signal_commands.connect(sigc::mem_fun(*this, &DevicePage::on_commands_update));
	
	// Now that the callbacks are all registered, we can connect.
	devctrl->connect();
}

DevicePage::~DevicePage() {
	log.term(format("%s", __PRETTY_FUNCTION__));

	// Destruct device control (if present)
	if (devctrl)
		delete devctrl;
}

void DevicePage::enable_gui() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	dev_cmds.set_sensitive(true);
	dev_val.set_sensitive(true);
	dev_send.set_sensitive(true);
}

void DevicePage::disable_gui() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	dev_cmds.set_sensitive(false);
	dev_val.set_sensitive(false);
	dev_send.set_sensitive(false);
}

void DevicePage::clear_gui() {
	log.term(format("%s", __PRETTY_FUNCTION__));

	dev_cmds.clear_items();
	dev_cmds.append_text("-");
	dev_stat.set_text("N/A");
}

void DevicePage::on_dev_send_activate() {
	log.term(format("%s", __PRETTY_FUNCTION__));

	// Send command from dev_cmds and dev_val
	string cmd = dev_cmds.get_active_text();
	cmd += " " + dev_val.get_text();
	devctrl->send_cmd(cmd);
}

void DevicePage::on_commands_update() {
	log.term(format("%s", __PRETTY_FUNCTION__));

	// Update list of device commands
	dev_cmds.clear_items();
	
	DeviceCtrl::cmdlist_t::iterator it;
	DeviceCtrl::cmdlist_t devcmd_l = devctrl->get_devcmds();

  for (it=devcmd_l.begin(); it!=devcmd_l.end(); ++it)
		dev_cmds.append_text(*it);
	
	// Add empty field to send complete freeform command as well
	dev_cmds.append_text(" ");
}

void DevicePage::on_message_update() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	if (devctrl->is_ok() || devctrl->is_calib()) {
		dev_stat.entry.modify_base(STATE_NORMAL, Gdk::Color("lightgreen"));
		dev_stat.set_text("Ok");
	}
	else {
		dev_stat.entry.modify_base(STATE_NORMAL, Gdk::Color("red"));
		dev_stat.set_text("Err: " + devctrl->get_errormsg());
	}	
}

void DevicePage::on_connect_update() {
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, devctrl->is_connected()));

	if (devctrl->is_connected())
		enable_gui();
	else
		disable_gui();
}

