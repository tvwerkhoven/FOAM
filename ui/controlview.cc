/*
 controlview.h -- FOAM GUI connection control pane
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
 @file controlview.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM connection control pane
 */

#include "format.h"
#include "foamcontrol.h"
#include "controlview.h"

using namespace std;
using namespace Gtk;

ControlPage::ControlPage(Log &log, FoamControl &foamctrl): 
log(log), foamctrl(foamctrl),
connframe("Connection"), host("Hostname"), port("Port"), connect("Connect"),
modeframe("Run mode"), mode_listen("Listen"), mode_open("Open loop"), mode_closed("Closed loop"), shutdown("Shutdown"),
calibframe("Calibration"), calmode_lbl("Calibration mode: "), calib("Calibrate"),
statframe("Status"), stat_mode("Mode: "), stat_nwfs("# WFS: "), stat_nwfc("# WFC: "), stat_nframes("# Frames: ") {
	
	// request minimum size for entry boxes
	host.set_size_request(120);
	host.set_text("localhost");
	port.set_size_request(50);
	port.set_text("1025");
	
	// Make shutdown/stop button red
	shutdown.modify_bg(STATE_NORMAL, Gdk::Color("red"));
	shutdown.modify_bg(STATE_ACTIVE, Gdk::Color("red"));
	shutdown.modify_bg(STATE_PRELIGHT, Gdk::Color("red"));
	shutdown.set_sensitive(false);

	// Disable buttons for now
	mode_listen.set_sensitive(false);
	mode_open.set_sensitive(false);
	mode_closed.set_sensitive(false);
	shutdown.set_sensitive(false);
	
	// Default selector
	calmode_select.append_text("-");
	calmode_select.set_sensitive(false);
	calib.set_sensitive(false);

	// Make status entries insensitive
	stat_mode.set_editable(false);
	stat_mode.set_size_request(75);
	stat_mode.set_text("-");
	stat_nwfs.set_editable(false);
	stat_nwfs.set_size_request(30);
	stat_nwfs.set_text("-");
	stat_nwfc.set_editable(false);
	stat_nwfc.set_size_request(30);
	stat_nwfc.set_text("-");
	stat_nframes.set_editable(false);
	stat_nframes.set_size_request(75);
	stat_nframes.set_text("-");
		
	// Connection row (hostname, port, connect button)
	connbox.set_spacing(4);	
	connbox.pack_start(host, PACK_SHRINK);
	connbox.pack_start(port, PACK_SHRINK);
	connbox.pack_start(connect, PACK_SHRINK);
	connframe.add(connbox);

	// Runmode row (listen, open, closed, shutdown)
	modebox.set_spacing(4);
	modebox.pack_start(mode_listen, PACK_SHRINK);
	modebox.pack_start(mode_open, PACK_SHRINK);
	modebox.pack_start(mode_closed, PACK_SHRINK);
	modebox.pack_start(shutdown, PACK_SHRINK);
	modeframe.add(modebox);
	
	// Calibration row
	calibbox.set_spacing(4);
	calibbox.pack_start(calmode_lbl, PACK_SHRINK);
	calibbox.pack_start(calmode_select, PACK_SHRINK);
	calibbox.pack_start(calib, PACK_SHRINK);
	calibframe.add(calibbox);
	
	// Status row (mode, # wfs, # wfc, # frames)
	statbox.set_spacing(4);
	statbox.pack_start(stat_mode, PACK_SHRINK);
	statbox.pack_start(stat_nwfs, PACK_SHRINK);
	statbox.pack_start(stat_nwfc, PACK_SHRINK);
	statbox.pack_start(stat_nframes, PACK_SHRINK);
	statframe.add(statbox);
	
	set_spacing(4);
	pack_start(connframe, PACK_SHRINK);
	pack_start(modeframe, PACK_SHRINK);
	pack_start(calibframe, PACK_SHRINK);
	pack_start(statframe, PACK_SHRINK);
	
	// register callback functions
	connect.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_connect_clicked));
	
	mode_listen.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_listen_clicked));
	mode_open.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_open_clicked));
	mode_closed.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_closed_clicked));
	shutdown.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_shutdown_clicked));
	
	calib.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_calib_clicked));	
	
	show_all_children();
}

ControlPage::~ControlPage() {
}

void ControlPage::on_connect_clicked() {
	printf("%dControlPage::on_connect_clicked()\n", (int) pthread_self());
	if (foamctrl.is_connected()) {
		log.add(Log::NORMAL, "Trying to disconnect");
		foamctrl.disconnect();
	}
	else {
		log.add(Log::NORMAL, "Trying to connect to " + host.get_text() + ":" + port.get_text());
		foamctrl.connect(host.get_text(), port.get_text());
	}
}

void ControlPage::on_mode_listen_clicked() {
	printf("%x:ControlPage::on_mode_listen_clicked()\n", (int) pthread_self());
	log.add(Log::NORMAL, "Setting mode listen...");
	foamctrl.set_mode(AO_MODE_LISTEN);
}

void ControlPage::on_mode_closed_clicked() {
	printf("%dControlPage::on_mode_closed_clicked()\n", (int) pthread_self());
	log.add(Log::NORMAL, "Setting mode closed...");
	foamctrl.set_mode(AO_MODE_CLOSED);
}

void ControlPage::on_mode_open_clicked() {
	printf("%x:ControlPage::on_mode_open_clicked()\n", (int) pthread_self());
	log.add(Log::NORMAL, "Setting mode open...");
	foamctrl.set_mode(AO_MODE_OPEN);
}

void ControlPage::on_shutdown_clicked() {
	printf("%x:ControlPage::on_shutdown_clicked()\n", (int) pthread_self());
	log.add(Log::NORMAL, "Trying to shutdown");
	foamctrl.shutdown();
}

void ControlPage::on_calib_clicked() {
	printf("%x:ControlPage::on_calmode_changed()\n", (int) pthread_self());
	log.add(Log::NORMAL, "Trying to calibrate");
	foamctrl.calibrate(calmode_select.get_active_text());
}

void ControlPage::on_connect_update() {
	printf("%x:ControlPage::on_connect_update()\n", (int) pthread_self());
	if (foamctrl.is_connected()) {
		printf("%x:ControlPage::on_connect_update() is conn\n", (int) pthread_self());
		connect.set_label("Disconnect");

		mode_listen.set_sensitive(true);
		mode_open.set_sensitive(true);
		mode_closed.set_sensitive(true);
		shutdown.set_sensitive(true);
		
		calmode_select.set_sensitive(true);
		calib.set_sensitive(true);
		
		log.add(Log::OK, "Connected to " + foamctrl.getpeername());
	}
	else {
		printf("%x:ControlPage::on_connect_update() is not conn\n", (int) pthread_self());
		connect.set_label("Connect");

		mode_listen.set_sensitive(false);
		mode_open.set_sensitive(false);
		mode_closed.set_sensitive(false);
		shutdown.set_sensitive(false);
		
		calmode_select.set_sensitive(false);
		calib.set_sensitive(false);
		
		log.add(Log::OK, "Disconnected");
	}
}

void ControlPage::on_message_update() {
	printf("%x:ControlPage::on_message_update()\n", (int) pthread_self());
	
	// reset buttons
	mode_listen.set_sensitive(true);
	mode_open.set_sensitive(true);
	mode_closed.set_sensitive(true);
	calib.set_sensitive(true);
	
	// press correct button
	if (foamctrl.get_mode() == AO_MODE_LISTEN) mode_listen.set_sensitive(false);
	else if (foamctrl.get_mode() == AO_MODE_OPEN) mode_open.set_sensitive(false);
	else if (foamctrl.get_mode() == AO_MODE_CLOSED) mode_closed.set_sensitive(false);
	else if (foamctrl.get_mode() == AO_MODE_CAL) calib.set_sensitive(false);

	// set values in status box
	// TODO: update wrt FOAM implementation
	stat_mode.set_text(foamctrl.get_mode_str());
	stat_nwfs.set_text(format("%d", foamctrl.get_numdev()));
	stat_nwfc.set_text(format("%d", foamctrl.get_numdev()));
	stat_nframes.set_text(format("%d", foamctrl.get_numframes()));
	
	// set values in calibmode select box
	// TODO: this is ugly
	calmode_select.clear_items();
	string *modetmp = foamctrl.get_calmodes();
	while (*modetmp != "__SENTINEL__") {
		calmode_select.prepend_text(*modetmp++);
	}
	calmode_select.set_active_text(*--modetmp);
}

