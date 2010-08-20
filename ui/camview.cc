#define __STDC_FORMAT_MACROS

#define GL_GLEXT_PROTOTYPES
#define GL_ARB_IMAGING

#include <stdexcept>
#include <cstring>
#include <gtkmm/accelmap.h>
#include "camview.h"
#include "glarea.h"

using namespace std;
using namespace Gtk;

void CamView::force_update() {
	glarea.crosshair = crosshair.get_active();
	//glarea.pager = pager.get_active();
	// Flip settings
	glarea.fliph = fliph.get_active();
	glarea.flipv = flipv.get_active();
	glarea.do_update();
}

void CamView::do_update() {
	// TODO
	glarea.do_update();
}

// TODO: what is this?
bool CamView::on_timeout() {
	if(waitforupdate && time(NULL) - lastupdate < 5)
		return true;

	auto frame = get_window();
	if(!frame || frame->get_state() == Gdk::WINDOW_STATE_WITHDRAWN || frame->get_state() == Gdk::WINDOW_STATE_ICONIFIED)
		return true;

	int x1, y1, x2, y2;

	double cw = camera.get_width();
	double ch = camera.get_height();
	double ww = image.get_width();
	double wh = image.get_height();
	double ws = fit.get_active() ? min(ww / cw, wh / ch) : pow(2.0, scale.get_value());
	int cs = round(pow(2.0, -scale.get_value()));

	// Ensure camera scale results in a texture width divisible by 4
	while(cs > 1 && ((int)cw / cs) & 0x3)
		cs--;
	if(cs < 1)
		cs = 1;

	int fx = fliph.get_active() ? -1 : 1;
	int fy = flipv.get_active() ? -1 : 1;

	// Convert window corners to camera coordinates, use 4 pixel safety margin
	x1 = (cw - ww / ws - fx * sx * cw) / 2 / cs - 4;
	y1 = (ch - wh / ws + fy * sy * ch) / 2 / cs - 4;
	x2 = (cw + ww / ws - fx * sx * cw) / 2 / cs + 7;
	y2 = (ch + wh / ws + fy * sy * ch) / 2 / cs + 4;

	// Align x coordinates to multiples of 4
	x1 &= ~0x3;
	x2 &= ~0x3;

	waitforupdate = true;
	lastupdate = time(NULL);
	camera.grab(x1, y1, x2, y2, cs, darkflat.get_active(), fsel.get_active() ? 10 : 0);

	return true;
}

bool CamView::on_histogram_clicked(GdkEventButton *event) {
	if(event->type != GDK_BUTTON_PRESS)
		return false;

	int shift = depth - 8;
	if(shift < 0)
		shift = 0;
	double value = event->x * (1 << shift);
	
	if(event->button == 1)
		minval.set_value(value);
	if(event->button == 2) {
		minval.set_value(value - (16 << shift));
		maxval.set_value(value + (16 << shift));
	}
	if(event->button == 3)
		maxval.set_value(value);

	contrast.set_active(false);
	force_update();

	return true;
}

void CamView::on_zoom1_activate() {
	fit.set_active(false);
	glarea.setscale(0.0);
}

void CamView::on_zoomin_activate() {
	fit.set_active(false);
	glarea.scalestep(1.0/3.0);
}

void CamView::on_zoomout_activate() {
	fit.set_active(false);
	glarea.scalestep(-1.0/3.0);
}

void CamView::on_histogram_toggled() {
	int w, h, fh;
	get_size(w, h);

	if(histogram.get_active()) {
		histogramframe.show();
		fh = histogramframe.get_height();
		resize(w, h + fh);
	} else {
		fh = histogramframe.get_height();
		histogramframe.hide();
		resize(w, h - fh);
	}
}

bool CamView::on_window_state_event(GdkEventWindowState *event) {
	config.set(camera.name + ".state", event->new_window_state);
	return true;
}

void CamView::on_window_configure_event(GdkEventConfigure *event) {
	if(get_window()->get_state())
		return;

	config.set(camera.name + ".x", event->x);
	config.set(camera.name + ".y", event->y);
	config.set(camera.name + ".w", event->width);
	config.set(camera.name + ".h", event->height);
}

CamView::CamView(Log &log, FoamControl &foamctrl, string n): DevicePage(log, foamctrl, n),
dispframe("Display settings"),
ctrlframe("Camera controls"),
camframe("Camera"),
histogramframe("Histogram"),
flipv("Flip vert."), fliph("Flip hor."), crosshair("Crosshair"), zoomin(Stock::ZOOM_IN), zoomout(Stock::ZOOM_OUT), zoom100(Stock::ZOOM_100), zoomfit(Stock::ZOOM_FIT), refresh(Stock::REFRESH),
mean("Mean value"), stddev("Std. deviation")
{
	lastupdate = 0;
	waitforupdate = false;
	histo = 0;
	s = -1;

	fliph.set_active(false);
	flipv.set_active(false);
	fit.set_active(false);
	crosshair.set_active(false);

	AccelMap::add_entry("<camview>/menu/view/fliph", AccelKey("h").get_key(), Gdk::SHIFT_MASK);
	AccelMap::add_entry("<camview>/menu/view/flipv", AccelKey("v").get_key(), Gdk::SHIFT_MASK);
	AccelMap::add_entry("<camview>/menu/view/zoomfit", AccelKey("f").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camview>/menu/view/zoom100", AccelKey("1").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camview>/menu/view/zoomin", AccelKey("plus").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camview>/menu/view/zoomout", AccelKey("minus").get_key(), Gdk::ModifierType(0));
	//AccelMap::add_entry("<camera>/menu/view/histogram", AccelKey("h").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/view/contrast", AccelKey("c").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/view/underover", AccelKey("e").get_key(), Gdk::ModifierType(0));
	AccelMap::add_entry("<camview>/menu/view/crosshair", AccelKey("c").get_key(), Gdk::SHIFT_MASK);
//	AccelMap::add_entry("<camera>/menu/view/fullscreen", AccelKey("F11").get_key(), Gdk::ModifierType(0));
	
//	AccelMap::add_entry("<camera>/menu/extra/darkflat", AccelKey("d").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/extra/fsel", AccelKey("s").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/extra/tiptilt", AccelKey("t").get_key(), Gdk::ModifierType(0));

	fliph.set_accel_path("<camview>/menu/view/fliph");
	flipv.set_accel_path("<camview>/menu/view/flipv");
	zoomfit.set_accel_path("<camview>/menu/view/zoomfit");
	zoom100.set_accel_path("<camview>/menu/view/zoom100");
	zoomin.set_accel_path("<camview>/menu/view/zoomin");
	zoomout.set_accel_path("<camview>/menu/view/zoomout");
//	histogram.set_accel_path("<camview>/menu/view/histogram");
//	contrast.set_accel_path("<camview>/menu/view/contrast");
//	underover.set_accel_path("<camview>/menu/view/underover");
	crosshair.set_accel_path("<camview>/menu/view/crosshair");
//	fullscreentoggle.set_accel_path("<camview>/menu/view/fullscreen");
//	darkflat.set_accel_path("<camview>/menu/extra/darkflat");
//	fsel.set_accel_path("<camview>/menu/extra/fsel");
//	tiptilt.set_accel_path("<camview>/menu/extra/tiptilt");

	mean.set_alignment(1);
	stddev.set_alignment(1);
	mean.set_editable(false);
	stddev.set_editable(false);

	// signals

//	signal_configure_event().connect_notify(sigc::mem_fun(*this, &CamView::on_window_configure_event));
//	image.signal_configure_event().connect_notify(sigc::mem_fun(*this, &CamView::on_image_configure_event));
//	image.signal_expose_event().connect_notify(sigc::mem_fun(*this, &CamView::on_image_expose_event));
//	image.signal_realize().connect(sigc::mem_fun(*this, &CamView::on_image_realize));
//	imageevents.signal_scroll_event().connect(sigc::mem_fun(*this, &CamView::on_image_scroll_event));
//	imageevents.signal_button_press_event().connect(sigc::mem_fun(*this, &CamView::on_image_button_event));
//	imageevents.signal_motion_notify_event().connect(sigc::mem_fun(*this, &CamView::on_image_motion_event));
//	imageevents.add_events(Gdk::POINTER_MOTION_HINT_MASK);
	
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &CamView::on_timeout), 1000.0/30.0);

	fliph.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
	flipv.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
	zoomfit.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
//	contrast.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
//	underover.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
	crosshair.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));

//	histogram.signal_toggled().connect(sigc::mem_fun(*this, &CamView::on_histogram_toggled));
	zoom100.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_zoom100_activate));
	zoomin.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_zoomin_activate));
	zoomout.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_zoomout_activate));
//	histogramevents.signal_button_press_event().connect(sigc::mem_fun(*this, &CamView::on_histogram_clicked));
//	fullscreentoggle.signal_toggled().connect(sigc::mem_fun(*this, &CamView::on_fullscreen_toggled));
//	close.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_close_activate));
//	colorsel.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_colorsel_activate));


	// layout
	disphbox.pack_start(flipv, PACK_SHRINK);
	disphbox.pack_start(flipv, PACK_SHRINK);
	disphbox.pack_start(zoomfit, PACK_SHRINK);
	disphbox.pack_start(zoom100, PACK_SHRINK);
	disphbox.pack_start(zoomin, PACK_SHRINK);
	disphbox.pack_start(zoomout, PACK_SHRINK);
	dispframe.add(disphbox);
	
	ctrlhbox.pack_start(refresh, PACK_SHRINK);
	ctrlframe.add(ctrlhbox);
	
	camhbox.pack_start(glarea);
	camframe.add(camhbox);
	
	histohbox.pack_start(mean, PACK_SHRINK);
	histohbox.pack_start(stddev, PACK_SHRINK);
	histohframe.add(histohbox);
	
	pack_start(dispframe, PACK_SHRINK);
	pack_start(ctrlframe, PACK_SHRINK);
	pack_start(camframe);
	pack_start(histoframe);

	// finalize
	show_all_children();
}

int CamView::init() {
	fprintf(stderr, "CamView::init()\n");
	camctrl = new CamCtrl();
	
	depth = camctrl->get_depth();
	
	camctrl->signal_monitor.connect(sigc::mem_fun(*this, &CamView::on_update));
}

CamView::~CameraMonitor() {
	// TODO: store configuration here?
}
