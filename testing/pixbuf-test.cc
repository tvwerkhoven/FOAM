/*
 *  pixbuf-test.cc
 *  foam
 *
 *  Created by Tim on 20100120.
 *  Copyright 2010 Tim van Werkhoven. All rights reserved.
 *
 */


#include <cstdio>
#include <cstring>
#include <vector>
#include <gtkmm.h>
#include <stdint.h>

using namespace std;
using namespace Gtk;

class CameraMonitor: public Window {
	VBox vbox;
	
	ScrolledWindow scroll;
	Image image;
	Button start;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	
	
public:
  bool on_timeout();
	void on_button();
	int m_timeout;
	void initimage();
	void mkimage();
	void drawimage();
	
	uint8_t *data;
	
	CameraMonitor();
	~CameraMonitor();
};
void CameraMonitor::initimage() {
	long i;
	for (i=0; i<640*480*3; i++) {
		data[i] = (uint8_t) (drand48() * 255.0 * i / (640*480*3));
	}
}

void CameraMonitor::mkimage() {
	long i;
	data[0] = data[640*480*3-1];
	for (i=1; i<640*480*3; i++) {
		data[i] = (data[i-1]+1 * 2) % 255;
	}
}

void CameraMonitor::drawimage() {
	unsigned char *outp = (unsigned char *)(pixbuf->get_pixels());
	for (long i=0; i<640*480*3; i++) {
		*outp++ = data[i];
	}
	
}	

void CameraMonitor::on_button() {
	printf("button! - %d\n", 640*480);
	mkimage();
	drawimage();
	image.queue_draw();
}

bool CameraMonitor::on_timeout() {
	printf("timeout!\n");
	
	mkimage();
	drawimage();
	image.queue_draw();
	
  //As this is a timeout function, return true so that it
  //continues to get called
  return true;
}

CameraMonitor::CameraMonitor():
start("update") {
	pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, 640, 480);
	pixbuf->fill(0x0000ff00);
	image.set(pixbuf);
	data = new uint8_t[640*480*3];
	initimage();
	
	set_title("Camera ");
	set_gravity(Gdk::GRAVITY_STATIC);


	scroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scroll.set_size_request(256, 256);
	scroll.set_double_buffered(false);
	image.set_double_buffered(false);
	
	start.signal_clicked().connect( sigc::mem_fun(*this ,&CameraMonitor::on_button) );

	scroll.add(image);
	vbox.pack_start(scroll);
	vbox.pack_end(start, PACK_SHRINK);

	add(vbox);

	// finalize

	show_all_children();
	
	m_timeout = Glib::signal_timeout().connect(sigc::mem_fun(*this, &CameraMonitor::on_timeout), 50 );
}

CameraMonitor::~CameraMonitor() {
	delete data;
}

int main(int argc, char *argv[]) {
	Glib::thread_init();
	Main kit(argc, argv);
	
	CameraMonitor *window = new CameraMonitor();
	Main::run(*window);
	
	delete window;
	return 0;
}

// EOF
