/*
 about.h -- FOAM GUI about window
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2009 Guus Sliepen <guus@sliepen.eu.org>
 
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

#ifndef HAVE_ABOUT_H
#define HAVE_ABOUT_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <vector>

class AboutFOAMGui: public Gtk::AboutDialog {
	std::vector<Glib::ustring> authors;
	Glib::RefPtr<Gdk::Pixbuf> logo;

	void on_response(int id);

	public:
	AboutFOAMGui();
};

#endif // HAVE_ABOUT_H
