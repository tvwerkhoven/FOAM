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

CamView::CamView(Log &log, FoamControl &foamctrl, string n, bool is_parent): 
DevicePage(log, foamctrl, n),
infoframe("Info"),
dispframe("Display settings"),
ctrlframe("Camera controls"),
camframe("Camera"),
histoframe("Histogram"),
e_exposure("Exp."), e_offset("Offset"), e_interval("Intv."), e_gain("Gain"), e_res("Res."), e_mode("Mode"), e_stat("Status"),
flipv("Flip vert."), fliph("Flip hor."), crosshair("Crosshair"), grid("Grid"), zoomin(Stock::ZOOM_IN), zoomout(Stock::ZOOM_OUT), zoom100(Stock::ZOOM_100), zoomfit(Stock::ZOOM_FIT), capture("Capture"), display("Display"), store("Store"),
mean("Mean value"), stddev("Stddev")
{
	fprintf(stderr, "%x:CamView::CamView()\n", (int) pthread_self());
	
	lastupdate = 0;
	waitforupdate = false;
	s = -1;
	
	e_exposure.set_width_chars(8);
	e_offset.set_width_chars(4);
	e_interval.set_width_chars(8);
	e_gain.set_width_chars(4);
	
	e_res.set_width_chars(12);
	e_res.set_editable(false);
	e_mode.set_width_chars(8);
	e_mode.set_editable(false);
	e_stat.set_width_chars(20);
	e_stat.set_editable(false);
	
	fliph.set_active(false);
	flipv.set_active(false);
	crosshair.set_active(false);
	grid.set_active(false);
	
	store_n.set_width_chars(4);
	
	mean.set_width_chars(6);
	mean.set_alignment(1);
	mean.set_editable(false);
	stddev.set_width_chars(6);
	stddev.set_alignment(1);
	stddev.set_editable(false);
	
	clear_gui();
	disable_gui();
	
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
	crosshair.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
	grid.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
	
	zoomfit.signal_toggled().connect(sigc::mem_fun(*this, &CamView::force_update));
	zoom100.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoom100_activate));
	zoomin.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoomin_activate));
	zoomout.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoomout_activate));

	capture.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_capture_clicked));
	display.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_display_clicked));
	store.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_store_clicked));
	
	// Handle some glarea events as well
	glarea.view_update.connect(sigc::mem_fun(*this, &CamView::on_glarea_view_update));
		
	// layout
	infohbox.set_spacing(4);
	infohbox.pack_start(e_exposure, PACK_SHRINK);
	infohbox.pack_start(e_offset, PACK_SHRINK);
	infohbox.pack_start(e_interval, PACK_SHRINK);
	infohbox.pack_start(e_gain, PACK_SHRINK);
	infohbox.pack_start(e_res, PACK_SHRINK);
	infohbox.pack_start(e_mode, PACK_SHRINK);
	infohbox.pack_start(e_stat, PACK_SHRINK);
	infoframe.add(infohbox);
	
	disphbox.set_spacing(4);
	disphbox.pack_start(flipv, PACK_SHRINK);
	disphbox.pack_start(fliph, PACK_SHRINK);
	disphbox.pack_start(crosshair, PACK_SHRINK);
	disphbox.pack_start(grid, PACK_SHRINK);
	disphbox.pack_start(vsep1, PACK_SHRINK);
	disphbox.pack_start(zoomfit, PACK_SHRINK);
	disphbox.pack_start(zoom100, PACK_SHRINK);
	disphbox.pack_start(zoomin, PACK_SHRINK);
	disphbox.pack_start(zoomout, PACK_SHRINK);
	dispframe.add(disphbox);
	
	//ctrlhbox.pack_start(refresh, PACK_SHRINK);
	ctrlhbox.set_spacing(4);
	ctrlhbox.pack_start(capture, PACK_SHRINK);
	ctrlhbox.pack_start(display, PACK_SHRINK);
	ctrlhbox.pack_start(store, PACK_SHRINK);
	ctrlhbox.pack_start(store_n, PACK_SHRINK);
	ctrlframe.add(ctrlhbox);
	
	camhbox.pack_start(glarea);
	camframe.add(camhbox);
	
	histohbox.set_spacing(4);
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
	
	if (is_parent)
		init();
}

CamView::~CamView() {
	//!< @todo store (gui) configuration here?
	fprintf(stderr, "%x:CamView::~CamView()\n", (int) pthread_self());
}

void CamView::enable_gui() {
	DevicePage::enable_gui();
	
	e_exposure.set_sensitive(true);
	e_offset.set_sensitive(true);
	e_interval.set_sensitive(true);
	e_gain.set_sensitive(true);
	
	fliph.set_sensitive(true);
	flipv.set_sensitive(true);
	crosshair.set_sensitive(true);
	grid.set_sensitive(true);
	
	capture.set_sensitive(true);
	display.set_sensitive(true);
	store.set_sensitive(true);
	store_n.set_sensitive(true);
	
	mean.set_sensitive(true);
	stddev.set_sensitive(true);
}

void CamView::disable_gui() {
	DevicePage::disable_gui();
	
	e_exposure.set_sensitive(false);
	e_offset.set_sensitive(false);
	e_interval.set_sensitive(false);
	e_gain.set_sensitive(false);
	
	fliph.set_sensitive(false);
	flipv.set_sensitive(false);
	crosshair.set_sensitive(false);
	grid.set_sensitive(false);

	capture.set_sensitive(false);
	display.set_sensitive(false);
	store.set_sensitive(false);
	store_n.set_sensitive(false);

	mean.set_sensitive(false);
	stddev.set_sensitive(false);
}

void CamView::clear_gui() {
	DevicePage::clear_gui();
	
	e_exposure.set_text("N/A");
	e_offset.set_text("N/A");
	e_interval.set_text("N/A");
	e_gain.set_text("N/A");
	e_res.set_text("N/A");
	e_mode.set_text("N/A");
	e_stat.set_text("N/A");

	capture.set_state(SwitchButton::CLEAR);
	display.set_state(SwitchButton::CLEAR);
	store.set_state(SwitchButton::CLEAR);
	
	store_n.set_text("10");

	mean.set_text("N/A");
	stddev.set_text("N/A");	
}

void CamView::init() {
	fprintf(stderr, "%x:CamView::init()\n", (int) pthread_self());
	// Init new camera control connection for this viewer
	camctrl = new CamCtrl(log, foamctrl.host, foamctrl.port, devname);
	// Downcast to generic device control pointer for base class (DevicePage in this case)
	devctrl = (DeviceCtrl *) camctrl;
	
	camctrl->signal_monitor.connect(sigc::mem_fun(*this, &CamView::on_monitor_update));
	camctrl->signal_message.connect(sigc::mem_fun(*this, &CamView::on_message_update));
	camctrl->signal_connect.connect(sigc::mem_fun(*this, &CamView::on_connect_update));
}

void CamView::on_glarea_view_update() {
	// Callback for glarea update on viewstate (zoom, scale, shift)
	zoomfit.set_active(glarea.getzoomfit());		
}

void CamView::force_update() {
	glarea.setcrosshair(crosshair.get_active());
	glarea.setgrid(grid.get_active());
	// Flip settings
	glarea.setfliph(fliph.get_active());
	glarea.setflipv(flipv.get_active());
	// Zoom settings
	glarea.setzoomfit(zoomfit.get_active());
	glarea.do_update();
}

void CamView::do_update() {
	//! @todo improve this
	glarea.do_update();
}

//! @todo what is this? do we need it?
bool CamView::on_timeout() {
	if(waitforupdate && time(NULL) - lastupdate < 5)
		return true;

	fprintf(stderr, "%x:CamView::on_timeout()\n", (int) pthread_self());

	return true;
}

void CamView::on_monitor_update() {
//	fprintf(stderr, "CamView::on_monitor_update()\n");
	//! @todo need mutex here?
	glarea.linkData((void *) camctrl->monitor.image, camctrl->monitor.depth, camctrl->monitor.x2 - camctrl->monitor.x1, camctrl->monitor.y2 - camctrl->monitor.y1);

	// Get new image if dispay button is in 'OK'
	if (display.get_state(SwitchButton::OK))
		camctrl->grab(0, 0, camctrl->get_width(), camctrl->get_height(), 1, false);
}

void CamView::on_connect_update() {
	fprintf(stderr, "%x:CamView::on_connect_update(conn=%d)\n", (int) pthread_self(), devctrl->is_connected());
	if (devctrl->is_connected())
		enable_gui();
	else
		disable_gui();
}

void CamView::on_message_update() {
	DevicePage::on_message_update();
	
	// Set values in text entries
	e_exposure.set_text(format("%g", camctrl->get_exposure()));
	e_offset.set_text(format("%g", camctrl->get_offset()));
	e_interval.set_text(format("%g", camctrl->get_interval()));
	e_gain.set_text(format("%g", camctrl->get_gain()));
	e_res.set_text(format("%dx%dx%d", camctrl->get_width(), camctrl->get_height(), camctrl->get_depth()));

	// Set 'Mode' text entry, change color appropriately
	e_mode.set_text(camctrl->get_modestr());
	if (camctrl->get_mode() == CamCtrl::WAITING || camctrl->get_mode() == CamCtrl::OFF)
		e_mode.entry.modify_base(STATE_NORMAL, Gdk::Color("orange"));
	else if (camctrl->get_mode() == CamCtrl::SINGLE || camctrl->get_mode() == CamCtrl::RUNNING)
		e_mode.entry.modify_base(STATE_NORMAL, Gdk::Color("lightgreen"));
	else
		e_mode.entry.modify_base(STATE_NORMAL, Gdk::Color("red"));
	
	if (camctrl->get_mode() == CamCtrl::OFF || camctrl->get_mode() == CamCtrl::WAITING)
		capture.set_state(SwitchButton::CLEAR);
	else if (camctrl->get_mode() == CamCtrl::CONFIG) 
		capture.set_state(SwitchButton::WAITING);
	else if (camctrl->get_mode() == CamCtrl::SINGLE || camctrl->get_mode() == CamCtrl::RUNNING) 
		capture.set_state(SwitchButton::OK);
	else
		capture.set_state(SwitchButton::ERROR);
	
	if (camctrl->is_ok()) {
		e_stat.entry.modify_base(STATE_NORMAL, Gdk::Color("lightgreen"));
		e_stat.set_text("Ok");
	}
	else {
		e_stat.entry.modify_base(STATE_NORMAL, Gdk::Color("red"));
		e_stat.set_text("Err: " + camctrl->get_errormsg());
	}	
	
	store_n.set_text(format("%d", camctrl->get_nstore()));
	if (camctrl->get_nstore() <= 0)
		store.set_state(SwitchButton::CLEAR);
	else
		store.set_state(SwitchButton::WAITING);

}

void CamView::on_info_change() {
	fprintf(stderr, "%x:CamView::on_info_change()\n", (int) pthread_self());
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

void CamView::on_capture_clicked() {
	if (camctrl->get_mode() == CamCtrl::RUNNING || 
			camctrl->get_mode() == CamCtrl::SINGLE) {
		fprintf(stderr, "%x:CamView::on_capture_update(): Stopping camera.\n", (int) pthread_self());
		camctrl->set_mode(CamCtrl::WAITING);
	}
	else {
		fprintf(stderr, "%x:CamView::on_capture_update(): Starting camera.\n", (int) pthread_self());
		camctrl->set_mode(CamCtrl::RUNNING);
	}
}

void CamView::on_display_clicked() {
	//! @bug Does not work when activated before capturing frames?
	if (display.get_state(SwitchButton::CLEAR)) {
		display.set_state(SwitchButton::OK);
		camctrl->grab(0, 0, camctrl->get_width(), camctrl->get_height(), 1, false);
	}
	else
		display.set_state(SwitchButton::CLEAR);
}

void CamView::on_store_clicked() {
	int nstore = atoi(store_n.get_text().c_str());
	fprintf(stderr, "%x:CamView::on_store_update() n=%d\n", (int) pthread_self(), nstore);
	
	if (nstore > 0 || nstore == -1)
		camctrl->store(nstore);
}
