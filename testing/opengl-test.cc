/*
 opengl-test.cc -- test OpenGL with GTK. Based on:
 simple.cc: Simple gtkglextmm example, written by Naofumi Yasufuku  <naofumi@users.sourceforge.net>
 
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

#include "glviewer.h"

using namespace std;
using namespace Gtk;

// Simple viewer class

class Simple: public Gtk::Window {
	Gtk::VBox vbox;
	Gtk::Button render;
	OpenGLImageViewer glarea;
public:
	void on_render();
	int w, h, d;
	uint8_t *data;
	Simple();
	~Simple();
};


Simple::Simple():
render("Re-render")
{
	fprintf(stderr, "Simple::Simple()\n");
	w = 100; h = 480; d = 8;
	data = (uint8_t *) malloc(w*h*d/8);
	on_render();
	
	set_title("OpenGL Window");
	
	set_gravity(Gdk::GRAVITY_STATIC);
	
  glarea.set_size_request(256, 256);

	render.signal_clicked().connect(sigc::mem_fun(*this, &Simple::on_render));

	vbox.pack_start(glarea);
	vbox.pack_start(render, PACK_SHRINK);
	add(vbox);
	

	show_all_children();
	
}

Simple::~Simple() {
	free(data);
}

void Simple::on_render() {
	fprintf(stderr, "Simple::on_render()\n");
	// fill data
	for (int i=0; i<w*h; i++)
		data[i] = drand48() * 255.0;
	
	glarea.linkData((void *) data, d, w, h);
}

int main(int argc, char** argv) {
	fprintf(stderr, "::main()\n");
  Gtk::Main kit(argc, argv);
  Gtk::GL::init(argc, argv);
	
  int major, minor;
  Gdk::GL::query_version(major, minor);
  std::cout << "OpenGL extension version - "
	<< major << "." << minor << std::endl;
	
  Simple simple;
	
  kit.run(simple);
	
  return 0;
}

