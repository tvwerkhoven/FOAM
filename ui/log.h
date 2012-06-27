/*
 log.h -- FOAM GUI log class
 Copyright (C) 2009--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_LOG_H
#define HAVE_LOG_H

#include <sys/time.h>
#include <gtkmm.h>
#include <queue>

#include "pthread++.h"

/*!
 @brief Graphical logging class
 
 GTK element using a Gtk::TextBuffer::TagTable to show timestamped logging
 messages in a GUI. This class is thread safe.
 */
class Log {
	public:
	enum severity {
		DEBUG,
		NORMAL,
		OK,
		WARNING,
		ERROR,
	};

	private:
	Glib::RefPtr<Gtk::TextBuffer::TagTable> tagtable;
	Glib::RefPtr<Gtk::TextBuffer> buffer;
	pthread::mutex mutex;

	struct entry {
		enum severity severity;
		struct timeval tv;
		Glib::ustring message;
	};

	std::queue<struct entry> entries;
	
	Glib::Dispatcher signal_update;
	void on_update();

	public:
	Log();
	~Log();

	Glib::RefPtr<Gtk::TextBuffer> &get_buffer();
	Glib::ustring get_text(); //!< Return the text in the log buffer
	void add(enum Log::severity severity, const Glib::ustring &message); //!< Add message to the log buffer.
	void term(std::string msg, bool showthread=true, FILE *stream=stderr); //!< Print message to a terminal (for higher volume/debugging purposes)
};

#endif // HAVE_LOG_H
