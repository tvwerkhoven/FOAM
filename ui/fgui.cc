/*
 fgui.cc -- the FOAM GUI
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
/*!
 @file fgui.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @brief This is the FOAM control GUI, consisting mostly of MainWindow::
 */

//! @bug crashes when running on expoao directly after connecting? Program received signal SIGSEGV, Segmentation fault. / 0x00007ffff2e8092e in XF86DRIQueryVersion () from /usr/lib/libGL.so.1

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#ifdef HAVE_GL_GL_H
#include "GL/gl.h"
#elif HAVE_OPENGL_GL_H
#include "OpenGL/gl.h"
#endif

#include <GL/glext.h>

#ifdef HAVE_GL_GLU_H
#include "GL/glu.h"
#elif HAVE_OPENGL_GLU_H 
#include "OpenGL/glu.h"
#endif

#ifdef HAVE_GL_GLUT_H
#include "GL/glut.h"
#elif HAVE_GLUT_GLUT_H 
#include "GLUT/glut.h"
#endif

#include <gtkmm.h>
#include <gtkmm/accelmap.h>
#include <gtkglmm.h>
#include <getopt.h>

#include <string.h>

#include <iostream>
#include <string>
#include <map>

#include "protocol.h"
#include "glviewer.h"
#include "sighandle.h"

#include "about.h"
#include "widgets.h"
#include "log.h"
#include "logview.h"
#include "foamcontrol.h"
#include "controlview.h"

// Supported devices go here
#include "deviceview.h"
#include "devicectrl.h"
#include "camview.h"
#include "camctrl.h"
#include "simcamview.h"
#include "simcamctrl.h"
#include "wfsview.h"
#include "wfsctrl.h"
#include "shwfsview.h"
#include "shwfsctrl.h"
#include "wfcview.h"
#include "wfcctrl.h"

#include "fgui.h"


extern Gtk::Tooltips *tooltips;

using namespace std;
using namespace Gtk;

const std::string FGUI_WELCOME = "FOAM Control (" PACKAGE_NAME " version " PACKAGE_VERSION " built " __DATE__ " " __TIME__ ")";
const std::string FGUI_COPYRIGHT = "Copyright (c) 2009--2011 " PACKAGE_BUGREPORT;

// !!!: ConnectDialog starts here

ConnectDialog::ConnectDialog(FoamControl &foamctrl): 
foamctrl(foamctrl), label("Connect to a remote host"), host("Hostname"), port("Port")
{
	set_title("Connect");
	set_modal();
	
	host.set_text("localhost");
	port.set_text("1025");
		
	add_button(Gtk::Stock::CONNECT, 1)->signal_clicked().connect(sigc::mem_fun(*this, &ConnectDialog::on_ok_clicked));
	add_button(Gtk::Stock::CANCEL, 0)->signal_clicked().connect(sigc::mem_fun(*this, &ConnectDialog::on_cancel_clicked));
	
	get_vbox()->add(label);
	get_vbox()->add(host);
	get_vbox()->add(port);
	
	show_all_children();
}

void ConnectDialog::on_ok_clicked() {
	printf("ConnectDialog::on_ok()\n");
	foamctrl.connect(host.get_text(), port.get_text());
	hide();
}

void ConnectDialog::on_cancel_clicked() {
	printf("ConnectDialog::on_cancel()\n");
	hide();
}

// !!!: MainMenu starts here

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

// !!!: MainWindow starts here
MainWindow::MainWindow(string &cfg, string &exec):
log(), foamctrl(log, cfg, exec), 
aboutdialog(), notebook(), conndialog(foamctrl), 
logpage(log), controlpage(log, foamctrl), 
menubar(*this) 
{
	log.add(Log::NORMAL, FGUI_WELCOME);
	log.add(Log::NORMAL, FGUI_COPYRIGHT);
	
	// widget properties
	set_title("FOAM Control");
	set_default_size(800, 300);
	set_gravity(Gdk::GRAVITY_STATIC);
	
	//vbox.set_spacing(4);
	//vbox.pack_start(menubar, PACK_SHRINK);
	
	// signals
	menubar.connect.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_connect_activate));
	menubar.quit.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_quit_activate));
	menubar.about.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about_activate));
		
	foamctrl.signal_connect.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_connect_update));
	foamctrl.signal_message.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_message_update));
	foamctrl.signal_device.connect(sigc::mem_fun(*this, &MainWindow::on_ctrl_device_update));	
		
	notebook.append_page(controlpage, "_Control", "Control", true);
	notebook.append_page(logpage, "_Log", "Log", true);
	
	vbox.pack_start(notebook);
	
	add(vbox);
	
	show_all_children();
	
	log.add(Log::OK, "FOAM Control up and running");
}

// !!!: Generic GUI Functions 

void MainWindow::disable_gui() {
	menubar.connect.set_sensitive(false);
}

void MainWindow::enable_gui() {
	menubar.connect.set_sensitive(true);
}

// !!!: MainWindow:button/GUI callbacks

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

// !!!: MainWindow:signal callbacks

void MainWindow::on_ctrl_connect_update() {
	fprintf(stderr, "MainWindow::on_ctrl_connect_update()\n");
	if (foamctrl.is_connected())
		disable_gui();
	else
		enable_gui();
}

void MainWindow::on_ctrl_message_update() {
	fprintf(stderr, "MainWindow::on_ctrl_message_update()\n");
}

void MainWindow::on_ctrl_device_update() {
	fprintf(stderr, "MainWindow::on_ctrl_device_update()\n");

	// Need mutex because we change this in both MainWindow:: fgui.cc and FoamControl::, asynchronously.
	pthread::mutexholder h(&(foamctrl.gui_mutex));
	
	DevicePage *tmppage=NULL;
	FoamControl::device_t *tmpdev=NULL;
	
	// First remove superfluous pages. Check all notebook pages and see if they exist in FoamControl:
	fprintf(stderr, "MainWindow::on_ctrl_device_update() deleting superfluous...\n");
	for (pagelist_t::iterator it = pagelist.begin(); it != pagelist.end(); ++it) {
		fprintf(stderr, "MainWindow::on_ctrl_device_update() %s\n", (it->first).c_str());
		tmppage = (DevicePage *) it->second;
		
		// Check if this exists in foamctrl. If not, remove
		tmpdev = foamctrl.get_device(tmppage);
		if (tmpdev == NULL) {
			fprintf(stderr, "MainWindow::on_ctrl_device_update() removing gui element\n");
			notebook.remove_page(*(tmppage)); // removes GUI element
			delete tmppage;
			pagelist.erase(it);
		}
	}
	
	// Check for each device from foamctrl if it is already a notebook page. If not, add.
	fprintf(stderr, "MainWindow::on_ctrl_device_update() adding new pages...\n");
	for (int i=0; i<foamctrl.get_numdev(); i++) {
		tmpdev = foamctrl.get_device(i);
		fprintf(stderr, "MainWindow::on_ctrl_device_update() %d/%d: %s - %s\n", i, foamctrl.get_numdev(), tmpdev->name.c_str(), tmpdev->type.c_str());
		
		if (pagelist.find(tmpdev->name) == pagelist.end()) {
			// This device is new! Init control and GUI element and add to mother GUI

			if (tmpdev->type.substr(0, 13) == "dev.wfs.shwfs") {
				fprintf(stderr, "MainWindow::on_ctrl_device_update() got shwfs device\n");
				tmpdev->ctrl = (DeviceCtrl *) new ShwfsCtrl(log, foamctrl.host, foamctrl.port, tmpdev->name);
				tmpdev->page = (DevicePage *) new ShwfsView((ShwfsCtrl *) tmpdev->ctrl, log, foamctrl, tmpdev->name);
				log.add(Log::OK, "Added new SH-WFS device, type="+tmpdev->type+", name="+tmpdev->name+".");
			}
			else if (tmpdev->type.substr(0, 7) == "dev.wfs") {
				fprintf(stderr, "MainWindow::on_ctrl_device_update() got generic wfs device\n");
				tmpdev->ctrl = (DeviceCtrl *) new WfsCtrl(log, foamctrl.host, foamctrl.port, tmpdev->name);
				tmpdev->page = (DevicePage *) new WfsView((WfsCtrl *) tmpdev->ctrl, log, foamctrl, tmpdev->name);
				log.add(Log::OK, "Added new generic WFS device, type="+tmpdev->type+", name="+tmpdev->name+".");
			}
			else if (tmpdev->type.substr(0, 16) == "dev.cam.simulcam") {
				log.term(format("%s got simulation cam dev", __PRETTY_FUNCTION__));
				
				tmpdev->ctrl = (DeviceCtrl *) new SimCamCtrl(log, foamctrl.host, foamctrl.port, tmpdev->name);
				tmpdev->page = (DevicePage *) new SimCamView((SimCamCtrl *) tmpdev->ctrl, log, foamctrl, tmpdev->name);
				log.add(Log::OK, "Added new simulation camera, type="+tmpdev->type+", name="+tmpdev->name+".");
			}
			else if (tmpdev->type.substr(0, 7) == "dev.cam") {
                log.term(format("%s got generic cam dev", __PRETTY_FUNCTION__));

				tmpdev->ctrl = (DeviceCtrl *) new CamCtrl(log, foamctrl.host, foamctrl.port, tmpdev->name);
				tmpdev->page = (DevicePage *) new CamView((CamCtrl *) tmpdev->ctrl, log, foamctrl, tmpdev->name);
				log.add(Log::OK, "Added new generic camera, type="+tmpdev->type+", name="+tmpdev->name+".");
			}
			else if (tmpdev->type.substr(0, 7) == "dev.wfc") {
				log.term(format("%s got generic wfc", __PRETTY_FUNCTION__));
				
				tmpdev->ctrl = (DeviceCtrl *) new WfcCtrl(log, foamctrl.host, foamctrl.port, tmpdev->name);
				tmpdev->page = (DevicePage *) new WfcView((WfcCtrl *) tmpdev->ctrl, log, foamctrl, tmpdev->name);
				log.add(Log::OK, "Added new generic wfc, type="+tmpdev->type+", name="+tmpdev->name+".");
			}
			// Fallback, if we don't have a good GUI element for the device, use a generic device controller
			else {
                log.term(format("%s got dev", __PRETTY_FUNCTION__));
				tmpdev->ctrl = (DeviceCtrl *) new DeviceCtrl(log, foamctrl.host, foamctrl.port, tmpdev->name);
				tmpdev->page = (DevicePage *) new DevicePage((DeviceCtrl *) tmpdev->ctrl, log, foamctrl, tmpdev->name);
				log.add(Log::OK, "Added new generic device, type="+tmpdev->type+", name="+tmpdev->name+".");
			}
			
			notebook.append_page(*(tmpdev->page), "_" + tmpdev->name, tmpdev->name, true);
			pagelist[tmpdev->name] = tmpdev->page;
      log.term(format("%s added dev", __PRETTY_FUNCTION__));
		}
	}
	
	show_all_children();
}


// !!!: General:Miscellaneous functions


static void signal_handler(int s) {
	if(s == SIGALRM || s == SIGPIPE)
		return;
	
	signal(s, SIG_DFL);
	
	fprintf(stderr, "fgui.cc::signal_handler(): Received %s signal, exitting\n", strsignal(s));
	
	if(s == SIGILL || s == SIGABRT || s == SIGFPE || s == SIGSEGV || s == SIGBUS)
		abort();
	else
		exit(s);
}

void show_version() {
	printf("%s\n", FGUI_WELCOME.c_str());
	printf("%s\n", FGUI_COPYRIGHT.c_str());
	printf("\nFOAM GUI comes with ABSOLUTELY NO WARRANTY. This is free software,\n"
				 "and you are welcome to redistribute it under certain conditions;\n"
				 "see the file COPYING for details.\n");
}

void show_clihelp(const string &execname, const bool error = false) {
	if(error)
		fprintf(stderr, "Try '%s --help' for more information.\n", execname.c_str());
	else {
		printf("Usage: %s [option]...\n\n", execname.c_str());
		printf("  -c, --config=FILE    Read configuration from FILE.\n"
					 "  -s, --sighandle=0|1  Toggle signal handling (default: 1).\n"
					 "  -h, --help           Display this help message.\n"
					 "      --version        Display version information.\n\n");
		printf("Report bugs to %s.\n", PACKAGE_BUGREPORT);
	}
}

int parse_args(int argc, char *argv[], string &conffile, bool &do_sighandle) {
	int r, option_index = 0;
	
	static struct option const long_options[] = {
		{"config", required_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 1},
		{"sighandle", required_argument, NULL, 's'},
		{NULL, 0, NULL, 0}
	};
	
	string execname(argv[0]);
	
	while((r = getopt_long(argc, argv, "c:hs:", long_options, &option_index)) != EOF) {
		switch(r) {
			case 0:
				break;
			case 'c':												// Configuration file
				conffile = string(optarg);
				break;
			case '?':												// Help
			case 'h':												// Help
				show_clihelp(execname);
				exit(0);
			case 1:													// Version info
				show_version();
				exit(0);
			case 's':												// Signal handling
				do_sighandle = bool(strtol(optarg, NULL, 10));
				break;
			default:
				show_clihelp(execname);
				exit(-1);
		}
	}
	
	return 0;
}



int main(int argc, char *argv[]) {
	// Parse command-line arguments
	string conffile("");
	string execname(argv[0]);
	bool do_sighandle(false);
	parse_args(argc, argv, conffile, do_sighandle);
			
	signal(SIGINT, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGPIPE, signal_handler);
	
	if (do_sighandle)
		SigHandle sighandler;

	Gtk::Main::init_gtkmm_internals();

	Glib::thread_init();
	
	Gtk::Main kit(argc, argv);
	// solve glibmm:ERROR:objectbase.cc:78:void Glib::ObjectBase::initialize(GObject*): assertion failed: (gobject_ == castitem) ?
	// http://stackoverflow.com/questions/4453399/cant-display-images-on-a-gtkmm-based-gnome-panel-applet
	Gtk::GL::init(argc, argv);
	
	glutInit(&argc, argv);
	
 	MainWindow *window = new MainWindow(conffile, execname);
	Main::run(*window);

	delete window;
	return 0;
}
