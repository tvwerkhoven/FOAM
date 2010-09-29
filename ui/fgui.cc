/*
 fgui.cc -- the FOAM GUI
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
 @file fgui.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief This is the FOAM control GUI

 @todo add timeout to connect() attempt
 @todo disable buttons when not connected.
 */

#include <gtkmm.h>
#include <gtkmm/accelmap.h>
#include <string.h>

#include <iostream>
#include <string>
#include <map>

#include "autoconfig.h"


#include "about.h"
#include "widgets.h"
#include "log.h"
#include "logview.h"
#include "protocol.h"
#include "foamcontrol.h"
#include "controlview.h"

#include "deviceview.h"
#include "camview.h"

#include "fgui.h"


extern Gtk::Tooltips *tooltips;

using namespace std;
using namespace Gtk;

void ConnectDialog::on_ok() {
	printf("ConnectDialog::on_ok()\n");
	foamctrl.connect(host.get_text(), port.get_text());
	hide();
}

void ConnectDialog::on_cancel() {
	printf("ConnectDialog::on_cancel()\n");
	hide();
}

ConnectDialog::ConnectDialog(FoamControl &foamctrl): 
foamctrl(foamctrl), label("Connect to a remote host"), host("Hostname"), port("Port")
{
	set_title("Connect");
	set_modal();
	
	host.set_text("localhost");
	port.set_text("1025");
		
	add_button(Gtk::Stock::CONNECT, 1)->signal_clicked().connect(sigc::mem_fun(*this, &ConnectDialog::on_ok));
	add_button(Gtk::Stock::CANCEL, 0)->signal_clicked().connect(sigc::mem_fun(*this, &ConnectDialog::on_cancel));
	
	get_vbox()->add(label);
	get_vbox()->add(host);
	get_vbox()->add(port);
	
	show_all_children();
}

MainMenu::MainMenu(Window &window):
file("File"), help("Help"),
connect(Stock::CONNECT), quit(Stock::QUIT), about(Stock::ABOUT) 
{
	// properties
	filemenu.set_accel_group(window.get_accel_group());
	helpmenu.set_accel_group(window.get_accel_group());
	
	filemenu.append(connect);
	filemenu.append(sep1);
	filemenu.append(quit);
	file.set_submenu(filemenu);
	
	helpmenu.append(about);
	help.set_submenu(helpmenu);
	
	add(file);
	add(help);
}


void MainWindow::on_about_activate() {
	printf("MainWindow::on_about_activate()\n");
	aboutdialog.present();
}
	
void MainWindow::on_quit_activate() {
	printf("MainWindow::on_quit_activate()\n");
	Main::quit();
}	

void MainWindow::on_connect_activate() {
	fprintf(stderr, "MainWindow::on_connect_activate()\n");
	conndialog.present();
}

void MainWindow::on_ctrl_connect_update() {
	fprintf(stderr, "MainWindow::on_ctrl_connect_update()\n");
	if (foamctrl.is_connected())
		menubar.connect.set_sensitive(false);
	else
		menubar.connect.set_sensitive(true);
	
	controlpage.on_connect_update();
}

void MainWindow::on_ctrl_message_update() {
	fprintf(stderr, "MainWindow::on_ctrl_message_update()\n");
	controlpage.on_message_update();
}

void MainWindow::on_ctrl_device_update() {
	fprintf(stderr, "MainWindow::on_ctrl_device_update()\n");
	fprintf(stderr, "MainWindow::on_ctrl_device_update() %d devs: ", foamctrl.get_numdev());
	for (int i=0; i<foamctrl.get_numdev(); i++) {
		FoamControl::device_t dev = foamctrl.get_device(i);
		fprintf(stderr, "%s - %s, ", dev.name.c_str(), dev.type.c_str());
	}
	fprintf(stderr, "\n");
	
	// We remove devices from the GUI that are not known to foamctrl here. 
	// We loop over devices in the GUI and if these do not appear in the 
	// foamctrl list, we remove them.
	devlist_t::iterator it;
	for (it=devlist.begin(); it != devlist.end(); it++) {
		// This GUI device is named it->first, has GUI element it->second.
		
		// Find this device and remove it if it does not exist
		int i=0;
		for (i=0; i<foamctrl.get_numdev(); i++) {
			FoamControl::device_t dev = foamctrl.get_device(i);
			if (dev.name == it->first) // Found it! break here now
				break;
			}
		if (i == foamctrl.get_numdev()) {
			// If i equals foamctrl.get_numdev(), the device wasn't found, remove it from the GUI
			//! @todo Add destructor to DevicePage to remove itself from the Notebook so we don't have to
			notebook.remove_page(*(it->second)); // removes GUI element
			delete it->second; // remove gui element itself
			devlist.erase(it); // remove from devlist
		}	
	}
	
	for (int i=0; i<foamctrl.get_numdev(); i++) {
		FoamControl::device_t dev = foamctrl.get_device(i);
		if (devlist.find(dev.name) != devlist.end())
			continue; // Already exists, skip

		// First check if type is sane
		if (dev.type.substr(0,3) != "dev") {
			fprintf(stderr, "MainWindow::on_ctrl_device_update() Type wrong!\n");
			continue;
		}
		// Then add specific devices first, and more general devices later
		else if (dev.type.substr(0,7) == "dev.cam") {
			fprintf(stderr, "MainWindow::on_ctrl_device_update() got generic camera device\n");
			CamView *tmp = new CamView(log, foamctrl, dev.name);
			tmp->init();
			devlist[dev.name] = (DevicePage *) tmp;
		}
		else if (dev.type.substr(0,3) == "dev") {
			fprintf(stderr, "MainWindow::on_ctrl_device_update() got generic device\n");			
			devlist[dev.name] = new DevicePage(log, foamctrl, dev.name);
		}
		else {
			fprintf(stderr, "MainWindow::on_ctrl_device_update() Type unknown!\n");
			continue;
		}
							
		log.add(Log::OK, "Found new device '" + dev.name + "' (type=" + dev.type + ").");
							
		notebook.append_page(*devlist[dev.name], "_" + dev.name, dev.name, true);
	}
	
	show_all_children();
}


MainWindow::MainWindow():
	log(), foamctrl(), 
	aboutdialog(), notebook(), conndialog(foamctrl), 
	logpage(log), controlpage(log, foamctrl), 
	menubar(*this) {
	log.add(Log::NORMAL, "FOAM Control (" PACKAGE_NAME " version " PACKAGE_VERSION " built " __DATE__ " " __TIME__ ")");
	log.add(Log::NORMAL, "Copyright (c) 2009 Tim van Werkhoven (T.I.M.vanWerkhoven@xs4all.nl)");
	
	// widget properties
	set_title("FOAM Control");
	set_default_size(640, 480);
	set_gravity(Gdk::GRAVITY_STATIC);
	
	vbox.set_spacing(4);
	vbox.pack_start(menubar, PACK_SHRINK);
	
	// signals
	menubar.connect.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_connect_activate));
	menubar.quit.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_quit_activate));
	menubar.about.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about_activate));
		
	foamctrl.signal_connect.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_connect_update));
	foamctrl.signal_message.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_message_update));
	foamctrl.signal_device.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_device_update));	
		
	controlpage.signal_device.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_device_update));
		
	notebook.append_page(controlpage, "_Control", "Control", true);
	notebook.append_page(logpage, "_Log", "Log", true);
	
	vbox.pack_start(notebook);
	
	add(vbox);
	
	show_all_children();
	
	log.add(Log::OK, "FOAM Control up and running");
}

static void signal_handler(int s) {
	if(s == SIGALRM || s == SIGPIPE)
		return;
	
	signal(s, SIG_DFL);
	
	fprintf(stderr, "Received %s signal, exitting\n", strsignal(s));
	
	if(s == SIGILL || s == SIGABRT || s == SIGFPE || s == SIGSEGV || s == SIGBUS)
		abort();
	else
		exit(s);
}

int main(int argc, char *argv[]) {
	signal(SIGINT, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGPIPE, signal_handler);
	
	Glib::thread_init();
	
	Gtk::Main kit(argc, argv);
	Gtk::GL::init(argc, argv);
	
 	MainWindow *window = new MainWindow();
	Main::run(*window);

	delete window;
	return 0;
}
