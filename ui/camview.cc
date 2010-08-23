/*
 camview.cc -- camera control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2010 Guus Sliepen
 
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#define GL_GLEXT_PROTOTYPES
#define GL_ARB_IMAGING

#include <stdexcept>
#include <cstring>
#include <gtkmm/accelmap.h>

#include "camctrl.h"
#include "glviewer.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

void CamView::force_update() {
	glarea.crosshair = crosshair.get_active();
	//glarea.pager = pager.get_active();
	// Flip settings
	glarea.fliph = fliph.get_active();
	glarea.flipv = flipv.get_active();
	// Zoom settings
	// TOOD: zoomfit!
	//glarea.fliph = zoomfit.get_active();
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

	fprintf(stderr, "CamView::on_timeout()\n");
//	auto frame = get_window();
//	if(!frame || frame->get_state() == Gdk::WINDOW_STATE_WITHDRAWN || frame->get_state() == Gdk::WINDOW_STATE_ICONIFIED)
//		return true;

//	int x1, y1, x2, y2;
//
//	double cw = camera.get_width();
//	double ch = camera.get_height();
//	double ww = image.get_width();
//	double wh = image.get_height();
//	double ws = fit.get_active() ? min(ww / cw, wh / ch) : pow(2.0, scale.get_value());
//	int cs = round(pow(2.0, -scale.get_value()));
//
//	// Ensure camera scale results in a texture width divisible by 4
//	while(cs > 1 && ((int)cw / cs) & 0x3)
//		cs--;
//	if(cs < 1)
//		cs = 1;
//
//	int fx = fliph.get_active() ? -1 : 1;
//	int fy = flipv.get_active() ? -1 : 1;
//
//	// Convert window corners to camera coordinates, use 4 pixel safety margin
//	x1 = (cw - ww / ws - fx * sx * cw) / 2 / cs - 4;
//	y1 = (ch - wh / ws + fy * sy * ch) / 2 / cs - 4;
//	x2 = (cw + ww / ws - fx * sx * cw) / 2 / cs + 7;
//	y2 = (ch + wh / ws + fy * sy * ch) / 2 / cs + 4;
//
//	// Align x coordinates to multiples of 4
//	x1 &= ~0x3;
//	x2 &= ~0x3;
//
//	waitforupdate = true;
//	lastupdate = time(NULL);
//	camera.grab(x1, y1, x2, y2, cs, darkflat.get_active(), fsel.get_active() ? 10 : 0);

	return true;
}

void CamView::on_message_update() {
	DevicePage::on_message_update();
}

void CamView::on_zoom100_activate() {
	zoomfit.set_active(false);
	glarea.setscale(0.0);
}

void CamView::on_zoomin_activate() {
	zoomfit.set_active(false);
	glarea.scalestep(1.0/3.0);
}

void CamView::on_zoomout_activate() {
	zoomfit.set_active(false);
	glarea.scalestep(-1.0/3.0);
}

CamView::~CamView() {
	// TODO: store configuration here?
}

CamView::CamView(Log &log, FoamControl &foamctrl, string n): 
DevicePage(log, foamctrl, n),
dispframe("Display settings"),
ctrlframe("Camera controls"),
camframe("Camera"),
histoframe("Histogram"),
flipv("Flip vert."), fliph("Flip hor."), crosshair("Crosshair"), zoomin(Stock::ZOOM_IN), zoomout(Stock::ZOOM_OUT), zoom100(Stock::ZOOM_100), zoomfit(Stock::ZOOM_FIT), refresh(Stock::REFRESH),
mean("Mean value"), stddev("Stddev")
{
	fprintf(stderr, "CamView::CamView()\n");
	
	lastupdate = 0;
	waitforupdate = false;
	s = -1;

	fliph.set_active(false);
	flipv.set_active(false);
	crosshair.set_active(false);
	
	mean.set_text("-");
	mean.set_width_chars(8);
	stddev.set_text("-");
	stddev.set_width_chars(8);

	// TODO: only works for menus
//	AccelMap::add_entry("<camview>/menu/view/fliph", AccelKey("h").get_key(), Gdk::SHIFT_MASK);
//	AccelMap::add_entry("<camview>/menu/view/flipv", AccelKey("v").get_key(), Gdk::SHIFT_MASK);
//	AccelMap::add_entry("<camview>/menu/view/zoomfit", AccelKey("f").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camview>/menu/view/zoom100", AccelKey("1").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camview>/menu/view/zoomin", AccelKey("plus").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camview>/menu/view/zoomout", AccelKey("minus").get_key(), Gdk::ModifierType(0));
	//AccelMap::add_entry("<camera>/menu/view/histogram", AccelKey("h").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/view/contrast", AccelKey("c").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/view/underover", AccelKey("e").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camview>/menu/view/crosshair", AccelKey("c").get_key(), Gdk::SHIFT_MASK);
//	AccelMap::add_entry("<camera>/menu/view/fullscreen", AccelKey("F11").get_key(), Gdk::ModifierType(0));
	
//	AccelMap::add_entry("<camera>/menu/extra/darkflat", AccelKey("d").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/extra/fsel", AccelKey("s").get_key(), Gdk::ModifierType(0));
//	AccelMap::add_entry("<camera>/menu/extra/tiptilt", AccelKey("t").get_key(), Gdk::ModifierType(0));

//	fliph.set_accel_path("<camview>/menu/view/fliph");
//	flipv.set_accel_path("<camview>/menu/view/flipv");
//	zoomfit.set_accel_path("<camview>/menu/view/zoomfit");
//	zoom100.set_accel_path("<camview>/menu/view/zoom100");
//	zoomin.set_accel_path("<camview>/menu/view/zoomin");
//	zoomout.set_accel_path("<camview>/menu/view/zoomout");
//	histogram.set_accel_path("<camview>/menu/view/histogram");
//	contrast.set_accel_path("<camview>/menu/view/contrast");
//	underover.set_accel_path("<camview>/menu/view/underover");
//	crosshair.set_accel_path("<camview>/menu/view/crosshair");
//	fullscreentoggle.set_accel_path("<camview>/menu/view/fullscreen");
//	darkflat.set_accel_path("<camview>/menu/extra/darkflat");
//	fsel.set_accel_path("<camview>/menu/extra/fsel");
//	tiptilt.set_accel_path("<camview>/menu/extra/tiptilt");

	mean.set_alignment(1);
	stddev.set_alignment(1);
	mean.set_editable(false);
	stddev.set_editable(false);

	// glarea
	//glarea.linkData((void *) NULL, 8, 0, 0);
	glarea.set_size_request(256, 256);	
	
	// signals

//	signal_configure_event().connect_notify(sigc::mem_fun(*this, &CamView::on_window_configure_event));
//	image.signal_configure_event().connect_notify(sigc::mem_fun(*this, &CamView::on_image_configure_event));
//	image.signal_expose_event().connect_notify(sigc::mem_fun(*this, &CamView::on_image_expose_event));
//	image.signal_realize().connect(sigc::mem_fun(*this, &CamView::on_image_realize));
//	imageevents.signal_scroll_event().connect(sigc::mem_fun(*this, &CamView::on_image_scroll_event));
//	imageevents.signal_button_press_event().connect(sigc::mem_fun(*this, &CamView::on_image_button_event));
//	imageevents.signal_motion_notify_event().connect(sigc::mem_fun(*this, &CamView::on_image_motion_event));
//	imageevents.add_events(Gdk::POINTER_MOTION_HINT_MASK);
	
	//Glib::signal_timeout().connect(sigc::mem_fun(*this, &CamView::on_timeout), 1000.0/30.0);

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
	disphbox.pack_start(fliph, PACK_SHRINK);
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
	histoframe.add(histohbox);
	
	pack_start(dispframe, PACK_SHRINK);
	pack_start(ctrlframe, PACK_SHRINK);
	pack_start(camframe);
	pack_start(histoframe, PACK_SHRINK);

	// finalize
	show_all_children();
}

int CamView::init() {
	fprintf(stderr, "CamView::init()\n");
	// Init new camera control connection for this viewer
	camctrl = new CamCtrl(foamctrl.host, foamctrl.port, devname);
	// Downcast to generic device control pointer for base class (DevicePage in this case)
	devctrl = (DeviceCtrl *) camctrl;
	
	depth = camctrl->get_depth();
	
	camctrl->signal_monitor.connect(sigc::mem_fun(*this, &CamView::force_update));
	camctrl->signal_update.connect(sigc::mem_fun(*this, &CamView::on_message_update));
	return 0;
}
