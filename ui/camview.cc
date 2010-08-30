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
#include <stdlib.h>

#include "camctrl.h"
#include "glviewer.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

CamView::CamView(Log &log, FoamControl &foamctrl, string n): 
DevicePage(log, foamctrl, n),
infoframe("Info"),
dispframe("Display settings"),
ctrlframe("Camera controls"),
camframe("Camera"),
histoframe("Histogram"),
e_exposure("Exp."), e_offset("Offset"), e_interval("Intv."), e_gain("Gain"), e_res("Res."), e_mode("Mode"), e_stat("Status"),
flipv("Flip vert."), fliph("Flip hor."), crosshair("Crosshair"), zoomin(Stock::ZOOM_IN), zoomout(Stock::ZOOM_OUT), zoom100(Stock::ZOOM_100), zoomfit(Stock::ZOOM_FIT), refresh(Stock::REFRESH),
mean("Mean value"), stddev("Stddev")
{
	fprintf(stderr, "CamView::CamView()\n");
	
	lastupdate = 0;
	waitforupdate = false;
	s = -1;
	
	e_exposure.set_text("N/A");
	e_exposure.set_width_chars(4);
	e_offset.set_text("N/A");
	e_offset.set_width_chars(4);
	e_interval.set_text("N/A");
	e_interval.set_width_chars(4);
	e_gain.set_text("N/A");
	e_gain.set_width_chars(4);
	e_res.set_text("N/A");
	e_res.set_width_chars(12);
	e_res.set_editable(false);
	e_mode.set_text("N/A");
	e_mode.set_width_chars(8);
	e_mode.set_editable(false);
	e_stat.set_text("N/A");
	e_stat.set_width_chars(12);
	e_stat.set_editable(false);
	
	fliph.set_active(false);
	flipv.set_active(false);
	crosshair.set_active(false);
	
	mean.set_text("N/A");
	mean.set_width_chars(6);
	mean.set_alignment(1);
	mean.set_editable(false);
	stddev.set_text("N/A");
	stddev.set_width_chars(6);
	stddev.set_alignment(1);
	stddev.set_editable(false);
	
	//! \todo AccelMap only works for menus, can we make shortcuts for buttons?	
	
	// glarea
	//glarea.linkData((void *) NULL, 8, 0, 0);
	glarea.set_size_request(256, 256);	
	
	// signals
	//Glib::signal_timeout().connect(sigc::mem_fun(*this, &CamView::on_timeout), 1000.0/30.0);
	e_exposure.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	e_offset.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	e_interval.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	e_gain.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	
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
	infohbox.pack_start(e_exposure, PACK_SHRINK);
	infohbox.pack_start(e_offset, PACK_SHRINK);
	infohbox.pack_start(e_interval, PACK_SHRINK);
	infohbox.pack_start(e_gain, PACK_SHRINK);
	infohbox.pack_start(e_res, PACK_SHRINK);
	infohbox.pack_start(e_mode, PACK_SHRINK);
	infohbox.pack_start(e_stat, PACK_SHRINK);
	infoframe.add(infohbox);
	
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
	
	pack_start(infoframe, PACK_SHRINK);
	pack_start(dispframe, PACK_SHRINK);
	pack_start(ctrlframe, PACK_SHRINK);
	pack_start(camframe);
	pack_start(histoframe, PACK_SHRINK);
	
	// finalize
	show_all_children();
}

CamView::~CamView() {
	//! \todo store (gui) configuration here?
}

void CamView::force_update() {
	glarea.crosshair = crosshair.get_active();
	//glarea.pager = pager.get_active();
	// Flip settings
	glarea.fliph = fliph.get_active();
	glarea.flipv = flipv.get_active();
	// Zoom settings
	//! \todo implement zoomfit in glarea
	//glarea.zoomfit = zoomfit.get_active();
	glarea.do_update();
}

void CamView::do_update() {
	//! \todo improve this
	glarea.do_update();
}

//! \todo what is this? do we need it?
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
	
	e_exposure.set_text(format("%g", camctrl->get_exposure()));
	e_offset.set_text(format("%g", camctrl->get_offset()));
	e_interval.set_text(format("%g", camctrl->get_interval()));
	e_gain.set_text(format("%g", camctrl->get_gain()));
	e_res.set_text(format("%dx%dx%d", camctrl->get_width(), camctrl->get_height(), camctrl->get_depth()));
	e_mode.set_text(camctrl->get_modestr());
	if (camctrl->is_ok()) {
		e_stat.entry.modify_base(STATE_NORMAL, Gdk::Color("green"));
		e_stat.set_text("Ok");
	}
	else {
		e_stat.entry.modify_base(STATE_NORMAL, Gdk::Color("red"));
		e_stat.set_text("Err: " + camctrl->get_errormsg());
	}
	
}

void CamView::on_info_change() {
	fprintf(stderr, "CamView::on_info_change()\n");
	camctrl->set_exposure(strtod(e_exposure.get_text().c_str(), NULL));
	camctrl->set_offset(strtod(e_offset.get_text().c_str(), NULL));
	camctrl->set_interval(strtod(e_interval.get_text().c_str(), NULL));
	camctrl->set_gain(strtod(e_gain.get_text().c_str(), NULL));
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

int CamView::init() {
	fprintf(stderr, "CamView::init()\n");
	// Init new camera control connection for this viewer
	camctrl = new CamCtrl(foamctrl.host, foamctrl.port, devname);
	// Downcast to generic device control pointer for base class (DevicePage in this case)
	devctrl = (DeviceCtrl *) camctrl;
	
//	depth = camctrl->get_depth();
	
	camctrl->signal_monitor.connect(sigc::mem_fun(*this, &CamView::force_update));
	camctrl->signal_update.connect(sigc::mem_fun(*this, &CamView::on_message_update));
	return 0;
}
