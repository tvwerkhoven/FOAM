#ifndef __ABOUT_H__
#define __ABOUT_H__

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

#endif // __ABOUT_H__
