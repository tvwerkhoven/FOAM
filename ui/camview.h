#ifndef HAVE_CAMERAMONITOR_H
#define HAVE_CAMERAMONITOR_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>
#include <vector>

#include "camera.h"
#include "widgets.h"

class CameraMonitor: public Gtk::Window {
	Gtk::VBox vbox;

	Glib::RefPtr<Gdk::GL::Config> glconfig;
	Glib::RefPtr<Gdk::GL::Window> glwindow;
	Gtk::GL::DrawingArea image;
	Gtk::EventBox imageevents;

	Gtk::Frame histogramframe;
	Gtk::VBox histogramvbox;
	Gtk::Alignment histogramalign;
	Gtk::EventBox histogramevents;
	Gtk::Image histogramimage;
	Glib::RefPtr<Gdk::Pixbuf> histogrampixbuf;
	LabeledSpinEntry scale;
	LabeledSpinEntry minval;
	LabeledSpinEntry maxval;
	LabeledEntry mean;
	LabeledEntry stddev;

	Gtk::MenuBar menubar;
	Gtk::MenuItem view;
	Gtk::MenuItem extra;

	Gtk::Menu viewmenu;
	Gtk::CheckMenuItem fliph;
	Gtk::CheckMenuItem flipv;
	Gtk::SeparatorMenuItem tsep1;
	Gtk::CheckMenuItem fit;
	Gtk::MenuItem zoom1;
	Gtk::MenuItem zoomin;
	Gtk::MenuItem zoomout;
	Gtk::SeparatorMenuItem tsep2;
	Gtk::CheckMenuItem histogram;
	Gtk::CheckMenuItem contrast;
	Gtk::SeparatorMenuItem tsep3;
	Gtk::CheckMenuItem underover;
	Gtk::CheckMenuItem crosshair;
	Gtk::SeparatorMenuItem tsep4;
	Gtk::CheckMenuItem fullscreentoggle;
	Gtk::ImageMenuItem close;

	Gtk::Menu extramenu;
	Gtk::MenuItem colorsel;
	Gtk::CheckMenuItem darkflat;
	Gtk::CheckMenuItem fsel;
	Gtk::CheckMenuItem tiptilt;

	void on_histogram_toggled();
	bool on_histogram_clicked(GdkEventButton *);
	bool on_window_state_event(GdkEventWindowState *event);
	void on_window_configure_event(GdkEventConfigure *event);
	void on_zoom1_activate();
	void on_zoomin_activate();
	void on_zoomout_activate();
	void on_colorsel_activate();
	void on_fullscreen_toggled();
	void force_update();
	void do_histo_update();
	void do_update();
	void on_close_activate();

	bool waitforupdate;
	time_t lastupdate;
	float dx;
	float dy;
	int s;
	float sx;
	float sy;
	uint32_t *histo;
	int depth;
	float sxstart;
	float systart;
	gdouble xstart;
	gdouble ystart;

	Glib::Dispatcher signal_update;
	void on_update();
	bool on_timeout();

	void on_image_realize();
	void on_image_expose_event(GdkEventExpose *event);
	void on_image_configure_event(GdkEventConfigure *event);
	bool on_image_scroll_event(GdkEventScroll *event);
	bool on_image_motion_event(GdkEventMotion *event);
	bool on_image_button_event(GdkEventButton *event);

	public:
	Camera &camera;
	CameraMonitor(Camera &camera);
	~CameraMonitor();
};

#endif
