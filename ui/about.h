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

#endif
