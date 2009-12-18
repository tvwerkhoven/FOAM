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

void Log::add(enum severity severity, const Glib::ustring &message) {
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
