/*
 logview.h -- FOAM GUI log viewing pane
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

#ifndef HAVE_LOGVIEW_H
#define HAVE_LOGVIEW_H

#include <gtkmm.h>
#include "log.h"

/*!
 @brief TODO Logging display class
 */
class LogPage: public Gtk::VBox {
	Gtk::ScrolledWindow scroll;
	Gtk::TextView view;
	Log &log;

	Gtk::HSeparator hsep;
	Gtk::CheckButton debug;

	void on_view_size_allocate(Gtk::Allocation &);
	void on_buffer_changed();
	void on_debug_toggled();

	public:
	LogPage(Log &log);
};

#endif // HAVE_LOGVIEW_H
