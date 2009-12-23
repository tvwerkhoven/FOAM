/* fgui.cc - the FOAM GUI
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
 @file fgui.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM control GUI
 */

#include <gtkmm.h>
#include <gtkmm/accelmap.h>
#include <string.h>

#include "autoconfig.h"

#include "about.h"
#include "fgui.h"
#include "log.h"
#include "logview.h"
#include "protocol.h"
#include "foamcontrol.h"
#include "controlview.h"

extern Gtk::Tooltips *tooltips;

using namespace std;
using namespace Gtk;

class ConnectDialog: public Dialog {
	FoamControl &foamctrl;
	
	Label host;
	Label port;
	Entry hostentry;
	Entry portentry;
	
	void on_ok() {
		printf("ConnectDialog::on_ok()\n");
		foamctrl.connect(hostentry.get_text(), portentry.get_text());
		
		hide();
	}
	
	void on_cancel() {
		printf("ConnectDialog::on_cancel()\n");
		hide();
	}
	
public:
	ConnectDialog(FoamControl &foamctrl): foamctrl(foamctrl), host("Hostname"), port("Port"), hostentry(), portentry() {
		set_title("Connect");
		set_modal();
		
		add_button(Gtk::Stock::OK, 1)->signal_clicked().connect(sigc::mem_fun(*this, &ConnectDialog::on_ok));
		add_button(Gtk::Stock::CANCEL, 0)->signal_clicked().connect(sigc::mem_fun(*this, &ConnectDialog::on_cancel));
		
		get_vbox()->add(host);
		get_vbox()->add(port);
		
		show_all_children();
	}
};

class MainMenu: public MenuBar {
	MenuItem file;
	MenuItem help;
	
	Menu filemenu;
	Menu helpmenu;
	
	SeparatorMenuItem sep1;
	SeparatorMenuItem sep2;
	
public:
	ImageMenuItem connect;
	ImageMenuItem quit;
	
	ImageMenuItem about;
	
	MainMenu(Window &window):
	file("File"), help("Help"),
	connect(Stock::CONNECT), quit(Stock::QUIT),
	about(Stock::ABOUT) {
		// properties
		
		filemenu.set_accel_group(window.get_accel_group());
		helpmenu.set_accel_group(window.get_accel_group());
		
		// layout
		
		filemenu.append(connect);
		filemenu.append(sep1);
		filemenu.append(quit);
		file.set_submenu(filemenu);
		
		helpmenu.append(about);
		help.set_submenu(helpmenu);
		
		add(file);
		add(help);
	}
};

class MainWindow: public Window {
	VBox vbox;
	AboutFOAMGui aboutdialog;
	Notebook notebook;
	Statusbar statusbar;
	
	Log log;
	
	LogPage logpage;
	ControlPage controlpage;
	
	void on_about_activate() {
		aboutdialog.present();
	}
	
	void on_quit_activate() {
		Main::quit();
	}	
	
	void on_connect_activate() {
		printf("MainWindow::on_connect_activate()\n");
	}	
	
public:	
	MainMenu menubar;
	
	MainWindow():
	logpage(log), controlpage(log), menubar(*this) {
		log.add(Log::NORMAL, "FOAM Control (" PACKAGE_NAME " version " PACKAGE_VERSION " built " __DATE__ " " __TIME__ ")");
		log.add(Log::NORMAL, "Copyright (c) 2009 Tim van Werkhoven (T.I.M.vanWerkhoven@xs4all.nl)\n");
		
		// widget properties
		set_title("FOAM Control");
		set_default_size(600, 400);
		set_gravity(Gdk::GRAVITY_STATIC);
		
		vbox.set_spacing(4);
		vbox.pack_start(menubar, PACK_SHRINK);
		
		// signals
		menubar.connect.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_connect_activate));
		menubar.quit.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_quit_activate));
		menubar.about.signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about_activate));
		
		notebook.append_page(controlpage, "_Control", "Control", true);
		notebook.append_page(logpage, "_Log", "Log", true);
		
		vbox.pack_start(notebook);
		
		add(vbox);
		
		show_all_children();
		
		log.add(Log::OK, "FOAM Control up and running");
	}
	
	~MainWindow() {
	}
};

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
	
	Main kit(argc, argv);
	
 	MainWindow *window = new MainWindow();
	Main::run(*window);

	delete window;
	return 0;
}
