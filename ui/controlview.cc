/*
 controlview.h -- FOAM GUI connection control pane
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
statframe("Status"), stat_mode("Mode: "), stat_ndev("# Dev: "), stat_nframes("# Frames: "), stat_lastcmd("Last cmd: ") {
	
	//set_spacing(4);
	
	// request minimum size for entry boxes
	host.set_width_chars(24);
	port.set_width_chars(5);
	
	// Make shutdown/stop button red
	shutdown.modify_bg(STATE_NORMAL, Gdk::Color("red"));
	shutdown.modify_bg(STATE_PRELIGHT, Gdk::Color("red"));

	stat_mode.set_width_chars(8);
	stat_ndev.set_width_chars(2);
	stat_nframes.set_width_chars(6);
	stat_lastcmd.set_width_chars(32);
	
	// Make status entries insensitive (always)
	stat_mode.set_editable(false);
	stat_mode.set_width_chars(8);
	stat_ndev.set_editable(false);
	stat_ndev.set_width_chars(2);
	stat_nframes.set_editable(false);
	stat_nframes.set_width_chars(6);
	stat_lastcmd.set_editable(false);
	stat_lastcmd.set_width_chars(32);
	
	// Disable GUI for now
	clear_gui();
	disable_gui();
	
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
	
	// Pack runmode and calibration mode in one HBox
	modecalibbox.set_spacing(4);
	modecalibbox.pack_start(modeframe, PACK_SHRINK);
	modecalibbox.pack_start(calibframe, PACK_SHRINK);
	
	// Status row (mode, # dev, # frames)
	statbox.set_spacing(4);
	statbox.pack_start(stat_ndev, PACK_SHRINK);
	statbox.pack_start(stat_nframes, PACK_SHRINK);
	statbox.pack_start(stat_lastcmd, PACK_SHRINK);
	statframe.add(statbox);
	
	pack_start(connframe, PACK_SHRINK);
	pack_start(modecalibbox, PACK_SHRINK);
	pack_start(statframe, PACK_SHRINK);
	
	// register callback functions
	connect.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_connect_clicked));
	//! @todo add handlers for host and port Entry widgets
	//host.signal_activated().connect(sigc::mem_fun(*this, &ControlPage::on_connect_clicked));
	//port.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_connect_clicked));
	
	mode_listen.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_listen_clicked));
	mode_open.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_open_clicked));
	mode_closed.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_closed_clicked));
	shutdown.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_shutdown_clicked));
	
	calib.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_calib_clicked));	
	
	// Register events
	foamctrl.signal_connect.connect(sigc::mem_fun(*this, &ControlPage::on_connect_update));
	foamctrl.signal_message.connect(sigc::mem_fun(*this, &ControlPage::on_message_update));
	
	// Start timeout signal to update some things continuously
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &ControlPage::on_timeout), 1000.0/CONTROLVIEW_UPD_RATE);

	
	// Show GUI & update contents
	show_all_children();
	on_message_update();
}

ControlPage::~ControlPage() {
}

/*!
 @brief Clear GUI elements when we disconnect ourselves
 */
void ControlPage::clear_gui() {
	// Reset connection fields
	host.set_text("localhost");
	port.set_text("1025");
	
	// Default selector
	calmode_select.clear_items();
	calmode_select.append_text("-");
	
	// Reset buttons
	mode_listen.set_state(SwitchButton::CLEAR);
	mode_open.set_state(SwitchButton::CLEAR);
	mode_closed.set_state(SwitchButton::CLEAR);
	calib.set_state(SwitchButton::CLEAR);
}

/*!
 @brief Disable GUI elements when we are disconnected
 */
void ControlPage::disable_gui() {
	mode_listen.set_sensitive(false);
	mode_open.set_sensitive(false);
	mode_closed.set_sensitive(false);
	shutdown.set_sensitive(false);
	
	calmode_select.set_sensitive(false);
	calib.set_sensitive(false);
}

/*!
 @brief Enable GUI elements when we are connected
 */
void ControlPage::enable_gui() {
	mode_listen.set_sensitive(true);
	mode_open.set_sensitive(true);
	mode_closed.set_sensitive(true);
	shutdown.set_sensitive(true);

	calmode_select.set_sensitive(true);
	calib.set_sensitive(true);
}

void ControlPage::on_connect_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
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
	log.term(format("%s", __PRETTY_FUNCTION__));
	log.add(Log::NORMAL, "Setting mode listen...");
	foamctrl.set_mode(AO_MODE_LISTEN);
}

void ControlPage::on_mode_closed_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	log.add(Log::NORMAL, "Setting mode closed...");
	foamctrl.set_mode(AO_MODE_CLOSED);
}

void ControlPage::on_mode_open_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	log.add(Log::NORMAL, "Setting mode open...");
	foamctrl.set_mode(AO_MODE_OPEN);
}

void ControlPage::on_shutdown_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	log.add(Log::NORMAL, "Trying to shutdown");
	foamctrl.shutdown();
}

void ControlPage::on_calib_clicked() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	log.add(Log::NORMAL, "Trying to calibrate");
	foamctrl.calibrate(calmode_select.get_active_text());
}

void ControlPage::on_connect_update() {
    log.term(format("%s (conn=%d)", __PRETTY_FUNCTION__, foamctrl.is_connected()));

	if (foamctrl.is_connected()) {
		log.add(Log::OK, "Connected to " + foamctrl.getpeername());
		connect.set_label("Disconnect");
		enable_gui();
	}
	else {
		log.add(Log::OK, "Disconnected");
		connect.set_label("Connect");
		disable_gui();
	}
}

void ControlPage::on_message_update() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	// reset mode buttons
	mode_listen.set_state(SwitchButton::CLEAR);
	mode_open.set_state(SwitchButton::CLEAR);
	mode_closed.set_state(SwitchButton::CLEAR);
	calib.set_state(SwitchButton::CLEAR);
	
	// Set correct button active
	SwitchButton *tmp=NULL;
	if (foamctrl.get_mode() == AO_MODE_LISTEN) tmp = &mode_listen;
	else if (foamctrl.get_mode() == AO_MODE_OPEN) tmp = &mode_open;
	else if (foamctrl.get_mode() == AO_MODE_CLOSED) tmp = &mode_closed;
	else if (foamctrl.get_mode() == AO_MODE_CAL) tmp = &calib;
	
	if (tmp) {
		if (foamctrl.is_ok())
			tmp->set_state(SwitchButton::OK);
		else
			tmp->set_state(SwitchButton::ERROR);
	}
	
	// set values in status box
	stat_mode.set_text(foamctrl.get_mode_str());
	stat_ndev.set_text(format("%d", foamctrl.get_numdev()));
	stat_nframes.set_text(format("%d", foamctrl.get_numframes()));

	if (foamctrl.is_ok()) 
		stat_lastcmd.modify_bg(STATE_ACTIVE , Gdk::Color("lightgreen"));
	else
		stat_lastcmd.modify_bg(STATE_ACTIVE , Gdk::Color("red"));
	stat_lastcmd.set_text(foamctrl.get_lastreply());
	
	// set values in calibmode select box
  Glib::ustring curitem = calmode_select.get_active_text();
	calmode_select.clear_items();
	for (size_t i=0; i<foamctrl.get_numcal(); i++)
		calmode_select.append_text(foamctrl.get_calmode(i));
	calmode_select.set_active_text(curitem);
}

bool ControlPage::on_timeout() {
	// Update 'frames' counter now
	//foamctrl.send_cmd("get frames");
	
	return true;
}
