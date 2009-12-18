#ifndef __LOG_H__
#define __LOG_H__

#include <sys/time.h>
#include <gtkmm.h>
#include <queue>

#include "pthread++.h"

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
	Glib::ustring get_text();
	void add(enum severity severity, const Glib::ustring &message);
};

#endif // __LOG_H__
