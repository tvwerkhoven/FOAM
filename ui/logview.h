#ifndef __LOGVIEW_H__
#define __LOGVIEW_H__

#include <gtkmm.h>
#include "log.h"

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

#endif // __LOGVIEW_H__