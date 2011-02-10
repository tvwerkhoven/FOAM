/*
 camview-test.cc -- test camera viewer, accept data from a protocol:: connection
 
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

#include <iostream>
#include <cstdlib>

#include <gtkmm.h>
#include <gtkglmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/accelmap.h>
#include <sigc++/signal.h>

#include "glviewer.h"
#include "protocol.h"
#include "socket.h"

using namespace std;
using namespace Gtk;
typedef Protocol::Server::Connection Connection;

// Simple camera viewer class
// +--------------------
// | Menubar
// viewer window

class CamView: public Gtk::Window {
	Gtk::VBox vbox;
	
	Gtk::MenuBar menubar;
	Gtk::MenuItem view;
	
	Gtk::Menu viewmenu;
	Gtk::CheckMenuItem fliph;
	Gtk::CheckMenuItem flipv;
	Gtk::SeparatorMenuItem tsep1;
	Gtk::CheckMenuItem fit;
	Gtk::ImageMenuItem zoom1;
	Gtk::ImageMenuItem zoomin;
	Gtk::ImageMenuItem zoomout;
	Gtk::SeparatorMenuItem tsep2;
	Gtk::CheckMenuItem crosshair;
	Gtk::CheckMenuItem pager;
	Gtk::SeparatorMenuItem tsep3;
	Gtk::ImageMenuItem close;
	
	Gtk::Frame glframe;
	OpenGLImageViewer glarea;
	
	Gtk::HBox hbox_button;
	Gtk::Button reset;
	Gtk::Button render;
	Gtk::Button zoom1b;
	Gtk::Button quit;
	
	Glib::Dispatcher signal_camview;
	Protocol::Server *serv1;
	void on_message(Connection *connection, std::string line);
	void on_connect(Connection *connection, bool status);

	void state_update();
	void on_zoom1_activate();
	void on_zoomin_activate();
	void on_zoomout_activate();
	void on_close_activate();
	
	void on_update();
	void on_render_clicked();
	void on_reset_clicked();
	void on_quit_clicked();
	
public:
	int w, h, d, size;
	uint8_t *data;
	
	CamView();
	~CamView();
};

CamView::CamView():
view("View"), fliph("Flip horizontal"), flipv("Flip vertical"),
zoom1(Stock::ZOOM_100), zoomin(Stock::ZOOM_IN), zoomout(Stock::ZOOM_OUT), 
crosshair("Show crosshair"), pager("Show pager"),
close(Stock::CLOSE),
glframe("Camera X"),
reset("Reset zoom/pan"), render(Stock::REFRESH), zoom1b(Stock::ZOOM_100), quit(Stock::QUIT)
{
	fprintf(stderr, "CamView::CamView()\n");
	
	// Setup 
	w = 200; h = 480; d = 8;
	size = w*h*d/8;
	//data = (uint8_t *) malloc(w*h*d/8);
	data = new uint8_t[size];
	on_render_clicked();
	
	set_title("OpenGL CamView window");
	set_gravity(Gdk::GRAVITY_STATIC);
	
	fliph.set_active(false);
	flipv.set_active(false);
	crosshair.set_active(false);
	pager.set_active(false);
	
	viewmenu.set_accel_group(get_accel_group());
	
	fliph.set_accel_path("<camera>/menu/view/fliph");
	AccelMap::add_entry("<camera>/menu/view/fliph", AccelKey("h").get_key(), Gdk::SHIFT_MASK);
	flipv.set_accel_path("<camera>/menu/view/flipv");
	AccelMap::add_entry("<camera>/menu/view/flipv", AccelKey("v").get_key(), Gdk::SHIFT_MASK);

	zoom1.set_accel_path("<camera>/menu/view/zoom1");
	AccelMap::add_entry("<camera>/menu/view/zoom1", AccelKey("1").get_key(), Gdk::ModifierType(0));
	zoomin.set_accel_path("<camera>/menu/view/zoomin");
	AccelMap::add_entry("<camera>/menu/view/zoomin", AccelKey("plus").get_key(), Gdk::ModifierType(0));
	zoomout.set_accel_path("<camera>/menu/view/zoomout");
	AccelMap::add_entry("<camera>/menu/view/zoomout", AccelKey("minus").get_key(), Gdk::ModifierType(0));
	
	crosshair.set_accel_path("<camera>/menu/view/crosshair");
	AccelMap::add_entry("<camera>/menu/view/crosshair", AccelKey("c").get_key(), Gdk::SHIFT_MASK);
	pager.set_accel_path("<camera>/menu/view/pager");
	AccelMap::add_entry("<camera>/menu/view/pager", AccelKey("p").get_key(), Gdk::SHIFT_MASK);

	glarea.set_size_request(256, 256);	

	// Connect callbacks
	fliph.signal_toggled().connect(sigc::mem_fun(*this, &CamView::state_update));
	flipv.signal_toggled().connect(sigc::mem_fun(*this, &CamView::state_update));
	crosshair.signal_toggled().connect(sigc::mem_fun(*this, &CamView::state_update));
	pager.signal_toggled().connect(sigc::mem_fun(*this, &CamView::state_update));
	zoom1.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_zoom1_activate));
	zoomin.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_zoomin_activate));
	zoomout.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_zoomout_activate));
	close.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_close_activate));
	
	reset.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_reset_clicked));
	render.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_render_clicked));
	zoom1b.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoom1_activate));
	quit.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_quit_clicked));
	
	// Signals
	
	signal_camview.connect(sigc::mem_fun(*this, &CamView::on_update));
	
	// Layout
	
	viewmenu.add(fliph);
	viewmenu.add(flipv);
	viewmenu.add(tsep1);
	viewmenu.add(zoom1);
	viewmenu.add(zoomin);
	viewmenu.add(zoomout);
	viewmenu.add(tsep2);
	viewmenu.add(crosshair);
	viewmenu.add(pager);
	viewmenu.add(tsep3);
	viewmenu.add(close);
	view.set_submenu(viewmenu);
	
	menubar.add(view);
	
	glframe.add(glarea);
	
	hbox_button.pack_start(reset);
	hbox_button.pack_start(render);
	hbox_button.pack_start(zoom1b);
	hbox_button.pack_start(quit);
	
	vbox.pack_start(menubar, PACK_SHRINK);
	vbox.pack_start(glframe);
	vbox.pack_start(hbox_button, PACK_SHRINK);
	add(vbox);
	
	show_all_children();
	
	printf("Starting server at port 1234, name CAM.\n");
	serv1 = new Protocol::Server("1234", "CAM");
	
	serv1->slot_message = sigc::mem_fun(this, &CamView::on_message);
	serv1->slot_connected = sigc::mem_fun(this, &CamView::on_connect);
	serv1->listen();
	
	//glarea.do_update();
}

CamView::~CamView() {
	free(data);
}

void CamView::state_update() {
	fprintf(stderr, "CamView::state_update()\n");
	glarea.setcrosshair(crosshair.get_active());
	glarea.setpager(pager.get_active());
	// Flip settings
	glarea.setfliph(fliph.get_active());
	glarea.setflipv(flipv.get_active());
	glarea.do_update();
}

void CamView::on_zoom1_activate() {
	fprintf(stderr, "CamView::on_zoom1_activate()\n");
	glarea.setscale(0.0);
}

void CamView::on_zoomin_activate() {
	fprintf(stderr, "CamView::on_zoomin_activate()\n");
	glarea.scalestep(1.0/3.0);
}

void CamView::on_zoomout_activate() {
	fprintf(stderr, "CamView::on_zoomout_activate()\n");
	glarea.scalestep(-1.0/3.0);
}

void CamView::on_close_activate() {
	fprintf(stderr, "CamView::on_close_activate()\n");
	Main::quit();
}

void CamView::on_reset_clicked() {
	fprintf(stderr, "CamView::on_reset_clicked()\n");
	glarea.setscale(0.0);
	glarea.setshift(0.0);
}

void CamView::on_render_clicked() {
	fprintf(stderr, "CamView::on_render_clicked()\n");
	
	for (int i=0; i<size; i++)
		data[i] = drand48() * 255.0;
	
	on_update();
}

void CamView::on_update() {
	fprintf(stderr, "CamView::on_update()\n");
	glarea.link_data((void *) data, d, w, h);
}

void CamView::on_quit_clicked() {
	fprintf(stderr, "CamView::on_quit_clicked()\n");
	Main::quit();
}


void CamView::on_connect(Connection *connection, bool status) {
	fprintf(stderr, "CamView:::on_connected: %d\n", status);
}

void CamView::on_message(Connection *connection, string line) {
	fprintf(stderr, "%s:on_message: %s\n", connection->server->name.c_str(), line.c_str());
	string word, cmd;
	
	
	cmd = popword(line);
	if (cmd == "IMG") {
		fprintf(stderr, "Getting image...\n");
		int isize = popint(line);
		int ix1 = popint(line);
		int iy1 = popint(line);
		int ix2 = popint(line);
		int iy2 = popint(line);
		int iscale = popint(line);
		
		if (size != isize) {
			size = isize;
			fprintf(stderr, "Realloc!\n");
			data = (uint8_t*) realloc(data, size);
		}
		
		w = ix2 - ix1;
		h = iy2 - iy1;
		
		fprintf(stderr, "Reading image s=%d, w=%d, h=%d...\n", size, w, h);
		connection->read(data, size);
		signal_camview();
	}
	else {
		fprintf(stderr, "serv:on_message: %s\n", cmd.c_str());
		while ((word = popword(line)).length())
			fprintf(stderr, "serv:on_message: %s\n", word.c_str());
	}
	
	fprintf(stderr, "writing back\n");
	connection->write("OK, got it");
}

int main(int argc, char** argv) {
	fprintf(stderr, "::main()\n");
  Gtk::Main kit(argc, argv);
  Gtk::GL::init(argc, argv);
	
  int major, minor;
  Gdk::GL::query_version(major, minor);
  std::cout << "OpenGL extension version - "
	<< major << "." << minor << std::endl;
	
  CamView camview;
	
  kit.run(camview);
	
  return 0;
}

