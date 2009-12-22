/* controlview.cc - the FOAM connection control pane
 Copyright (C) 2009 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
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

ControlPage::ControlPage(Log &log): 
log(log), foamctrl(*this), 
connframe("Connection"), modeframe("Run mode"),
host("Hostname"), port("Port"), connect("Connect"), disconnect("Disconnect"), 
hostentry(), portentry(), cspacer(),
mode_idle("Idle"), mode_open("Open loop"), mode_closed("Closed loop"), shutdown(Stock::STOP), mspacer() {
	
	// register callback functions
	connect.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_connect_clicked));
	disconnect.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_disconnect_clicked));
	
	shutdown.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_shutdown_clicked));
	mode_idle.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_idle_clicked));
	mode_open.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_open_clicked));
	mode_closed.signal_clicked().connect(sigc::mem_fun(*this, &ControlPage::on_mode_closed_clicked));
	
	// Make shutdown/stop button red
	shutdown.modify_bg(STATE_NORMAL, Gdk::Color("red"));
	shutdown.modify_bg(STATE_ACTIVE, Gdk::Color("red"));
	shutdown.modify_bg(STATE_PRELIGHT, Gdk::Color("red"));
	
	// request minimum size for entry boxes
	hostentry.set_size_request(120);
	hostentry.set_text("sirius1");
	portentry.set_size_request(50);
	portentry.set_text("1025");
	
	// Connection row (hostname, port, connect button)
	connbox.set_spacing(4);	
	connbox.pack_start(host, PACK_SHRINK);
	connbox.pack_start(hostentry, PACK_SHRINK);
	connbox.pack_start(port, PACK_SHRINK);
	connbox.pack_start(portentry, PACK_SHRINK);
	connbox.pack_start(connect, PACK_SHRINK);
	connbox.pack_start(disconnect, PACK_SHRINK);
	connbox.pack_end(cspacer);
	connect.show();
	disconnect.hide();
	connframe.add(connbox);

	// Runmode row (idle, open, closed, shutdown)
	modebox.set_spacing(4);
	modebox.pack_start(mode_idle, PACK_SHRINK);
	modebox.pack_start(mode_open, PACK_SHRINK);
	modebox.pack_start(mode_closed, PACK_SHRINK);
	modebox.pack_start(shutdown, PACK_SHRINK);
	modebox.pack_end(mspacer);
	modeframe.add(modebox);
	shutdown.set_sensitive(false);
	
	set_spacing(4);
	pack_start(connframe, PACK_SHRINK);
	pack_start(modeframe, PACK_SHRINK);
	
	on_connect_update();
	on_message_update();
}

ControlPage::~ControlPage() {
}

void ControlPage::on_connect_clicked() {
	printf("%d:ControlPage::on_connect_clicked()\n", pthread_self());
	log.add(Log::NORMAL, "Trying to connect to " + hostentry.get_text() + ":" + portentry.get_text());
	foamctrl.connect(hostentry.get_text(), portentry.get_text());
}

void ControlPage::on_disconnect_clicked() {
	printf("%dControlPage::on_disconnect_clicked()\n", pthread_self());
	log.add(Log::NORMAL, "Trying to disconnect");
	foamctrl.disconnect();
}

void ControlPage::on_mode_idle_clicked() {
	printf("%dControlPage::on_mode_idle_clicked()\n", pthread_self());
	log.add(Log::NORMAL, "Setting mode idle...");
	foamctrl.set_mode(AO_MODE_LISTEN);
}

void ControlPage::on_mode_closed_clicked() {
	printf("%dControlPage::on_mode_closed_clicked()\n", pthread_self());
	log.add(Log::NORMAL, "Setting mode closed...");
	foamctrl.set_mode(AO_MODE_CLOSED);
}

void ControlPage::on_mode_open_clicked() {
	printf("%dControlPage::on_mode_open_clicked()\n", pthread_self());
	log.add(Log::NORMAL, "Setting mode open...");
	foamctrl.set_mode(AO_MODE_OPEN);
}

void ControlPage::on_shutdown_clicked() {
	printf("%dControlPage::on_shutdown_clicked()\n", pthread_self());
	log.add(Log::NORMAL, "Trying to shutdown");
	foamctrl.shutdown();
}

void ControlPage::on_connect_update() {
	printf("%d:ControlPage::on_connect_update()\n", pthread_self());
	if (foamctrl.is_connected()) {
		printf("%d:ControlPage::on_connect_update() is conn\n", pthread_self());
		mode_idle.set_sensitive(true);
		mode_open.set_sensitive(true);
		mode_closed.set_sensitive(true);
		shutdown.set_sensitive(true);
		connect.hide();
		disconnect.show();
		log.add(Log::OK, "Connected to " + foamctrl.gethost() + ":" + foamctrl.getport());
	}
	else {
		printf("%d:ControlPage::on_connect_update() is not conn\n", pthread_self());
		mode_idle.set_sensitive(false);
		mode_open.set_sensitive(false);
		mode_closed.set_sensitive(false);
		shutdown.set_sensitive(false);
		disconnect.hide();
		connect.show();
		log.add(Log::OK, "Disconnected");
	}
}

void ControlPage::on_message_update() {
	printf("%d:ControlPage::on_message_update()\n", pthread_self());
	
}

