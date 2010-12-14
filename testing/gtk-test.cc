/*
 *  gtk-test.cc
 *  foam
 *
 *  Created by Tim on 20101124.
 *  Copyright 2010 Tim van Werkhoven. All rights reserved.
 *
 */


#include <cstdio>
#include <cstring>
#include <vector>
#include <gtkmm.h>

using namespace std;
using namespace Gtk;

class SwitchButton: public Button {
private:
	void modify_button(Button &button, const Gdk::Color &color) {
		button.modify_bg(STATE_PRELIGHT, color);
		button.modify_bg(STATE_NORMAL, color);
	}	
public:
	enum state {
		OK,
		READY,
		WARNING,
		WAITING,
		ERROR,
		OFF
	} state;							//!< Track state of the button
	const Gdk::Color col_ok;
	const Gdk::Color col_warn;
	const Gdk::Color col_err;
	
	void set_state(enum state s) {
		state = s;
		switch (s) {
			case OK:
			case READY:
				modify_button(*this, col_ok);
				break;
			case WARNING:
			case WAITING:
				modify_button(*this, col_warn);
				break;
			case OFF:
			case ERROR:
				modify_button(*this, col_err);
				break;
			default:
				break;
		}
	}
	enum state get_state() { return state; }
	
	SwitchButton(const Glib::ustring &lbl="Button"): 
	Button(lbl), 
	col_ok(Gdk::Color("lightgreen")), 
	col_warn(Gdk::Color("yellow")), 
	col_err(Gdk::Color("red"))
	{ 
		set_state(OK); 
	}
};
	
class Testwindow: public Window {
	VBox vbox;
	HBox hbox;
	
	Button b_ok;
	Button b_warn;
	Button b_err;
	SwitchButton tbutt2;
	
public:
	void on_b_ok();
	void on_b_warn();
	void on_b_err();

	void on_tbutton1();
	void on_tbutton2();

	Testwindow();
	~Testwindow();
};

void Testwindow::on_b_ok() {
	printf("on_button::signal_clicked()\n");
	tbutt2.set_state(SwitchButton::OK);
}
void Testwindow::on_b_warn() {
	printf("on_button::signal_clicked()\n");
	tbutt2.set_state(SwitchButton::WARNING);
}
void Testwindow::on_b_err() {
	printf("on_button::signal_clicked()\n");
	tbutt2.set_state(SwitchButton::ERROR);
}

void Testwindow::on_tbutton1() {
	printf("on_tbutton::signal_clicked()\n");
	tbutt2.set_state(SwitchButton::WAITING);
}
void Testwindow::on_tbutton2() {
	printf("on_tbutton::signal_activated()\n");
	tbutt2.set_state(SwitchButton::ERROR);
}


Testwindow::Testwindow():
b_ok("Set OK"), b_warn("Set WARN"), b_err("Set ERR"), tbutt2() {
	set_title("Test window");
	set_gravity(Gdk::GRAVITY_STATIC);


	b_ok.signal_clicked().connect( sigc::mem_fun(*this, &Testwindow::on_b_ok) );
	b_warn.signal_clicked().connect( sigc::mem_fun(*this, &Testwindow::on_b_warn) );
	b_err.signal_clicked().connect( sigc::mem_fun(*this, &Testwindow::on_b_err) );

	tbutt2.signal_clicked().connect( sigc::mem_fun(*this, &Testwindow::on_tbutton1) );
	tbutt2.signal_activate().connect( sigc::mem_fun(*this, &Testwindow::on_tbutton2) );
	
	hbox.pack_start(b_ok, PACK_SHRINK);
	hbox.pack_start(b_warn, PACK_SHRINK);
	hbox.pack_start(b_err, PACK_SHRINK);
	
	vbox.pack_start(tbutt2, PACK_SHRINK);
	vbox.pack_start(hbox, PACK_SHRINK);

	add(vbox);

	// finalize

	show_all_children();
}

Testwindow::~Testwindow() {
}

int main(int argc, char *argv[]) {
	Glib::thread_init();
	Main kit(argc, argv);
	
	Testwindow *window = new Testwindow();
	Main::run(*window);
	
	delete window;
	return 0;
}

// EOF