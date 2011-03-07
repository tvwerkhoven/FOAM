/*
 logview.cc -- FOAM GUI log viewing pane
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

#include "logview.h"

using namespace std;
using namespace Gtk;

LogPage::LogPage(Log &log): log(log), debug("Show debug messages") {
	// widget properties
	
	Glib::RefPtr<TextBuffer::TagTable> tagtable = log.get_buffer()->get_tag_table();
	tagtable->lookup("timestamp")->property_foreground() = "grey";
	tagtable->lookup("ok")->property_background() = "lightgreen";
	tagtable->lookup("warning")->property_background() = "orange";
	tagtable->lookup("error")->property_background() = "red";

	view.set_buffer(log.get_buffer());
	view.set_editable(false);
	scroll.set_policy(POLICY_AUTOMATIC, POLICY_ALWAYS);
	debug.set_active();

	// signals

	view.signal_size_allocate().connect(sigc::mem_fun(*this, &LogPage::on_view_size_allocate));
	//log.get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &LogPage::on_buffer_changed));
	debug.signal_toggled().connect(sigc::mem_fun(*this, &LogPage::on_debug_toggled));

	// layout

	scroll.add(view);

	pack_start(scroll);
	pack_end(debug, PACK_SHRINK);
	pack_end(hsep, PACK_SHRINK);
}

void LogPage::on_view_size_allocate(Allocation &) {
	Adjustment *v = scroll.get_vadjustment();
	if(v && v->get_value() > v->get_upper() - v->get_page_size() * 2)
		v->set_value(v->get_upper() - v->get_page_size());
}

void LogPage::on_buffer_changed() {
	TextBuffer::iterator end = log.get_buffer()->end();
	view.scroll_to(end);
}

void LogPage::on_debug_toggled() {
	log.get_buffer()->get_tag_table()->lookup("debug")->property_invisible() = !debug.get_active();
	TextBuffer::iterator end = log.get_buffer()->end();
	view.scroll_to(end);
}
