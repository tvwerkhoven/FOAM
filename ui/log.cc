/*
 log.cc -- FOAM GUI log class
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

#include "format.h"
#include "log.h"

using namespace std;
using namespace Gtk;

Log::Log() {
	tagtable = TextBuffer::TagTable::create();
	buffer = TextBuffer::create(tagtable);
	buffer->create_tag("timestamp");
	buffer->create_tag("debug");
	buffer->create_tag("normal");
	buffer->create_tag("ok");
	buffer->create_tag("warning");
	buffer->create_tag("error");

	signal_update.connect(sigc::mem_fun(*this, &Log::on_update));
}

Log::~Log() {
}

void Log::add(enum Log::severity severity, const Glib::ustring &message) {
	{
		pthread::mutexholder h(&mutex);
		struct entry e;
		e.severity = severity;
		e.message = message;
		gettimeofday(&e.tv, 0);
		entries.push(e);
	}

	signal_update();
}

void Log::term(string msg, bool showthread, FILE *stream) {
	if (showthread) {
		pthread_t pt = pthread_self();
		unsigned char *ptc = (unsigned char*)(void*)(&pt);
		char thrid[2+2*sizeof(pt)+1];
		sprintf(thrid, "0x");
		for (size_t i=0; i<sizeof(pt); i++)
			sprintf(thrid, "%s%02x", thrid, (unsigned)(ptc[i]));
		
		fprintf(stream, "(%s) %s\n", thrid, msg.c_str());
	} else {
		fprintf(stream, "%s\n", msg.c_str());
	}
}

Glib::RefPtr<TextBuffer> &Log::get_buffer() {
	return buffer;
}

void Log::on_update() {
	pthread::mutexholder h(&mutex);

	while(!entries.empty()) {
		struct entry &e = entries.front();
		Glib::ustring tag;
		if(e.severity == NORMAL)
			tag = "normal";
		else if(e.severity == OK)
			tag = "ok";
		else if(e.severity == WARNING)
			tag = "warning";
		else if(e.severity == ERROR)
			tag = "error";
		else
			tag = "debug";

		struct tm tm;
		gmtime_r(&e.tv.tv_sec, &tm);
		string timestamp = format("%04d-%02d-%02d %02d:%02d:%02d.%03d ", 1900 + tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, e.tv.tv_usec / 1000);
		Glib::RefPtr<TextBuffer::Mark> start = buffer->create_mark(buffer->end());
		buffer->insert_with_tag(buffer->end(), timestamp, "timestamp");
		buffer->insert(buffer->end(), e.message + '\n');
		buffer->apply_tag_by_name(tag, start->get_iter(), buffer->end());
		entries.pop();
	}
}
