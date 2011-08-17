/*
 camview.cc -- camera control class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
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

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#define GL_GLEXT_PROTOTYPES
#define GL_ARB_IMAGING

#include <stdexcept>
#include <cstring>
#include <gtkmm/accelmap.h>
#include <stdlib.h>
#include <math.h>

#include "utils.h"
#include "camctrl.h"
#include "glviewer.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

//! @bug High framerate: clicking 'display' to stop displaying does not work properly.
//! @todo Frame grabbing can be improved, we only need a subsection when zoomed in.


CamView::CamView(CamCtrl *camctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) camctrl, log, foamctrl, n), camctrl(camctrl),
ctrlframe("Camera controls"),
dispframe("Display settings"),
camframe("Camera " + devname),
histoframe("Histogram"),
capture("Capture"), display("Display"), store("Store"), e_exposure("Exp."), e_offset("Offset"), e_interval("Intv."), e_gain("Gain"), e_res("Res."), e_mode("Mode"),
flipv("Flip V"), fliph("Flip H"), crosshair("+"), grid("Grid"), histo("Histo"), underover("Underover"), 
zoomin(Stock::ZOOM_IN), zoomout(Stock::ZOOM_OUT), zoom100(Stock::ZOOM_100), zoomfit(Stock::ZOOM_FIT), 
histoalign(0.5, 0.5, 0, 0), minval("Display min"), maxval("Display max"), e_avg("Avg."), e_rms("RMS"), e_datamin("Min"), e_datamax("Max")
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	lastupdate = 0;
	waitforupdate = false;
	s = -1;
	histo_scale_f = LINEAR;
	
	// Setup histogram
	histo_img = 0;
	histopixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, 256, 100);
	histopixbuf->fill(0xFFFFFF00);
	histoimage.set(histopixbuf);
	histoimage.set_double_buffered(false);
	
	e_exposure.set_width_chars(4);
	e_offset.set_width_chars(4);
	e_interval.set_width_chars(4);
	e_gain.set_width_chars(4);
	
	e_res.set_width_chars(10);
	e_res.set_editable(false);
	e_mode.set_width_chars(8);
	e_mode.set_editable(false);
	
	fliph.set_active(false);
	flipv.set_active(false);
	crosshair.set_active(false);
	grid.set_active(false);
	histo.set_active(false);
	underover.set_active(false);
	
	store_n.set_width_chars(4);
	
	minval.set_range(0, 1 << camctrl->get_depth());
	minval.set_digits(0);
	minval.set_increments(1, 16);
	maxval.set_range(0, 1 << camctrl->get_depth());
	maxval.set_digits(0);
	maxval.set_increments(1, 16);
	
	e_avg.set_width_chars(6);
	e_avg.set_alignment(1);
	e_avg.set_editable(false);
	e_rms.set_width_chars(6);
	e_rms.set_alignment(1);
	e_rms.set_editable(false);
	e_datamin.set_width_chars(5);
	e_datamin.set_alignment(1);
	e_datamin.set_editable(false);
	e_datamax.set_width_chars(5);
	e_datamax.set_alignment(1);
	e_datamax.set_editable(false);
	
	clear_gui();
	disable_gui();
	
	glarea.set_size_request(256, 256);	
	
	// signals
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &CamView::on_timeout), 1000.0/10.0);
	capture.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_capture_clicked));
	display.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_display_clicked));
	store.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_store_clicked));
	store_n.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_store_clicked));

	e_exposure.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	e_offset.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	e_interval.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	e_gain.entry.signal_activate().connect(sigc::mem_fun(*this, &CamView::on_info_change));
	
	fliph.signal_toggled().connect(sigc::mem_fun(*this, &CamView::do_update));
	flipv.signal_toggled().connect(sigc::mem_fun(*this, &CamView::do_update));
	crosshair.signal_toggled().connect(sigc::mem_fun(*this, &CamView::do_update));
	grid.signal_toggled().connect(sigc::mem_fun(*this, &CamView::do_update));
	histo.signal_toggled().connect(sigc::mem_fun(*this, &CamView::on_histo_toggled));
	underover.signal_toggled().connect(sigc::mem_fun(*this, &CamView::on_underover_toggled));
	
	zoomfit.signal_toggled().connect(sigc::mem_fun(*this, &CamView::do_update));
	zoom100.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoom100_activate));
	zoomin.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoomin_activate));
	zoomout.signal_clicked().connect(sigc::mem_fun(*this, &CamView::on_zoomout_activate));
	
	histoevents.signal_button_press_event().connect(sigc::mem_fun(*this, &CamView::on_histo_clicked));
	minval.signal_value_changed().connect(sigc::mem_fun(*this, &CamView::on_minmax_change));
	maxval.signal_value_changed().connect(sigc::mem_fun(*this, &CamView::on_minmax_change));
	
	// Handle some glarea events as well
	glarea.view_update.connect(sigc::mem_fun(*this, &CamView::on_glarea_view_update));
		
	// Camera controls
	ctrlhbox.set_spacing(4);
	ctrlhbox.pack_start(capture, PACK_SHRINK);
	ctrlhbox.pack_start(display, PACK_SHRINK);
	ctrlhbox.pack_start(store, PACK_SHRINK);
	ctrlhbox.pack_start(store_n, PACK_SHRINK);
	ctrlhbox.pack_start(ctrl_vsep, PACK_SHRINK);
	ctrlhbox.pack_start(e_exposure, PACK_SHRINK);
	ctrlhbox.pack_start(e_offset, PACK_SHRINK);
	ctrlhbox.pack_start(e_interval, PACK_SHRINK);
	ctrlhbox.pack_start(e_gain, PACK_SHRINK);
	ctrlhbox.pack_start(e_res, PACK_SHRINK);
	ctrlhbox.pack_start(e_mode, PACK_SHRINK);
	ctrlframe.add(ctrlhbox);
	
	disphbox.set_spacing(4);
	disphbox.pack_start(flipv, PACK_SHRINK);
	disphbox.pack_start(fliph, PACK_SHRINK);
	disphbox.pack_start(crosshair, PACK_SHRINK);
	disphbox.pack_start(grid, PACK_SHRINK);
	disphbox.pack_start(histo, PACK_SHRINK);
	disphbox.pack_start(underover, PACK_SHRINK);
	disphbox.pack_start(vsep1, PACK_SHRINK);
	disphbox.pack_start(zoomfit, PACK_SHRINK);
	disphbox.pack_start(zoom100, PACK_SHRINK);
	disphbox.pack_start(zoomin, PACK_SHRINK);
	disphbox.pack_start(zoomout, PACK_SHRINK);
	dispframe.add(disphbox);
	
	// Camera window
	camframe.add(glarea);
	
	histoevents.add(histoimage);
	histoalign.add(histoevents);
	
	histohbox2.pack_start(e_avg, PACK_SHRINK);
	histohbox2.pack_start(e_rms, PACK_SHRINK);
	
	histohbox3.pack_start(e_datamin, PACK_SHRINK);
	histohbox3.pack_start(e_datamax, PACK_SHRINK);
	
	histovbox.pack_start(histohbox2);
	histovbox.pack_start(histohbox3);
	histovbox.pack_start(minval);
	histovbox.pack_start(maxval);

	histohbox.pack_start(histoalign);
	histohbox.pack_start(histovbox, PACK_SHRINK);

	histoframe.add(histohbox);
	
	pack_start(ctrlframe, PACK_SHRINK);
	pack_start(dispframe, PACK_SHRINK);
	
	// Extra window
	extra_win.set_title("FOAM Camera " + devname);
	
	extra_vbox.pack_start(camframe);
	extra_vbox.pack_start(histoframe, PACK_SHRINK);
	extra_win.add(extra_vbox);

	extra_win.show_all_children();
	extra_win.present();

	// Finalize
	show_all_children();
		
	histoframe.hide();
			
	camctrl->signal_monitor.connect(sigc::mem_fun(*this, &CamView::on_monitor_update));
}

CamView::~CamView() {
	//!< @todo store (gui) configuration here?
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void CamView::enable_gui() {
	DevicePage::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	e_exposure.set_sensitive(true);
	e_offset.set_sensitive(true);
	e_interval.set_sensitive(true);
	e_gain.set_sensitive(true);
	
	fliph.set_sensitive(true);
	flipv.set_sensitive(true);
	crosshair.set_sensitive(true);
	grid.set_sensitive(true);
	histo.set_sensitive(true);
	underover.set_sensitive(true);
	
	capture.set_sensitive(true);
	display.set_sensitive(true);
	store.set_sensitive(true);
	store_n.set_sensitive(true);
}

void CamView::disable_gui() {
	DevicePage::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	e_exposure.set_sensitive(false);
	e_offset.set_sensitive(false);
	e_interval.set_sensitive(false);
	e_gain.set_sensitive(false);
	
	fliph.set_sensitive(false);
	flipv.set_sensitive(false);
	crosshair.set_sensitive(false);
	grid.set_sensitive(false);
	histo.set_sensitive(false);
	underover.set_sensitive(false);

	capture.set_sensitive(false);
	display.set_sensitive(false);
	store.set_sensitive(false);
	store_n.set_sensitive(false);
}

void CamView::clear_gui() {
	DevicePage::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	e_exposure.set_text("N/A");
	e_offset.set_text("N/A");
	e_interval.set_text("N/A");
	e_gain.set_text("N/A");
	e_res.set_text("N/A");
	e_mode.set_text("N/A");
	
	capture.set_state(SwitchButton::CLEAR);
	display.set_state(SwitchButton::CLEAR);
	store.set_state(SwitchButton::CLEAR);
	
	store_n.set_text("10");

	e_avg.set_text("N/A");
	e_rms.set_text("N/A");	
	e_datamin.set_text("N/A");
	e_datamax.set_text("N/A");
	minval.set_value(0);
	maxval.set_value(1 << camctrl->get_depth());
}

void CamView::on_glarea_view_update() {
	zoomfit.set_active(glarea.getzoomfit());
}

void CamView::do_update() {
	glarea.setcrosshair(crosshair.get_active());
	glarea.setgrid(grid.get_active());
	// Flip settings
	glarea.setfliph(fliph.get_active());
	glarea.setflipv(flipv.get_active());
	// Zoom settings
	glarea.setzoomfit(zoomfit.get_active());
	
	glarea.do_update();
}

void CamView::on_histo_toggled() {
	int w, h, fh;
	extra_win.get_size(w, h);
	
	if (histo.get_active()) {
		histoframe.show();
		fh = histoframe.get_height();
		extra_win.resize(w, h + fh);
	} else {
		fh = histoframe.get_height();
		extra_win.resize(w, h - fh);
		histoframe.hide();
	}
}

void CamView::on_underover_toggled() {
	glarea.setunderover(underover.get_active());
	glarea.do_update();
}


int CamView::histo_scale_func(int max) {
	switch (histo_scale_f) {
		case LINEAR:
			return max;
		case SQRT:
			return sqrt(max*100);
			// Increasing steepness of the function:
		case LOG2:
			return log2((max*1.0/100.0)+1.)*100;
		case LOG10:
			return log10((max*9.0/100.0)+1.)*100;
		case LOG20:
			return log10((max*19.0/100.0)+1.)*100/log10(20.0);
		case LOG100:
			return log10((max*99.0/100.0)+1.)*100/log10(100.0);
		default:
			return max;
	}
}

void CamView::do_histo_update() {	
	int pixels = 0;											// Number of pixels
	size_t max = 1 << camctrl->get_depth(); // Maximum intensity in image
	double sum = 0;
	double sumsquared = 0;
	double rms = max;
	bool overexposed = false;						//!< Overexposed flag
	
	// Don't do anything is histoframe is hidden
	if (!histoframe.is_visible())
		return;
	
	// Analyze histogram data if available. histo is a linear array from 0 to
	// the maximum intensity and counts pixels for each intensity bin.
	if (histo_img) {
		for (int i = 0; i < max; i++) {
			pixels += histo_img[i];
			sum += (double)i * histo_img[i];
			sumsquared += (double)(i * i) * histo_img[i];
			
			if (i >= 0.98 * max && histo_img[i])
				overexposed = true;
		}
		
		sum /= pixels;
		sumsquared /= pixels;
		rms = sqrt(sumsquared - sum * sum) / sum;
	}
	
	// Update ranges
	minval.set_range(0, 1 << camctrl->get_depth());
	maxval.set_range(0, 1 << camctrl->get_depth());
	
	// Update min/max/avg/rms data values
	e_datamin.set_text(format("%d", camctrl->monitor.min));
	e_datamax.set_text(format("%d", camctrl->monitor.max));
	e_avg.set_text(format("%.2lf", sum));
	e_rms.set_text(format("%.3lf", rms));

	// Draw the histogram
	
	// total nr of pixels: pixels
	// maximum intensity in image (nr of bins): max
	// avg nr of pixels/bin: pixels / max
	// with average filled bin, the bar height will be ~0.1
	const int hscale = 1 + 10 * pixels / max;
	
	// Color red if overexposed
	if (overexposed && underover.get_active())
		histopixbuf->fill(0xff000000);
	else
		histopixbuf->fill(0xffffff00);
	
	// Draw black bars in the histogram
	uint8_t *out = (uint8_t *)histopixbuf->get_pixels();
	
	if(histo_img) {
		for(int i = 0; i < max; i++) {
			int height = histo_scale_func(histo_img[i] * 100 / hscale);
			if(height > 100)
				height = 100;
			for(int y = 100 - height; y < 100; y++) {
				uint8_t *p = out + 3 * ((i * 256 / max) + 256 * y);
				p[0] = 0;
				p[1] = 0;
				p[2] = 0;
			}
		}
	}
	
	// Make vertical bars (red and cyan) at minval and maxval:
	int x1 = clamp(minval.get_value_as_int() * 256 / max, (size_t) 0, (size_t) (1 << camctrl->get_depth()) - 1);
	int x2 = clamp(maxval.get_value_as_int() * 256 / max, (size_t) 0, (size_t) (1 << camctrl->get_depth()) - 1);
	
	for(int y = 0; y < 100; y += 2) {
		uint8_t *p = out + 3 * (x1 + 256 * y);
		p[0] = 255;
		p[1] = 0;
		p[2] = 0;
		p = out + 3 * (x2 + 256 * y);
		p[0] = 0;
		p[1] = 255;
		p[2] = 255;
	}
	
	histoimage.queue_draw();
}

bool CamView::on_timeout() {
	// Frame capture timed out, perhaps we need a new frame depending on 'display' state:
	// -> if WAITING: we're already waiting for a frame, pass
	// -> if OK: we are displaying a frame, get a new one & set to WAITING
	// -> if ERROR or CLEAR: don't do anything
	if (display.get_state() == SwitchButton::OK) {
		camctrl->grab(0, 0, camctrl->get_width(), camctrl->get_height(), 1, false, histo.get_active());
		display.set_state(SwitchButton::WAITING);
	}

	return true;
}

void CamView::on_monitor_update() {
	log.term(format("%s", __PRETTY_FUNCTION__));
	// New image has arrived, display & update 'display' state
	// -> if WAITING: we wanted the frame, update button to OK
	// -> if OK: no action necessary
	// -> if ERROR or CLEAR: don't do anything
	
	if (display.get_state() == SwitchButton::WAITING)
		display.set_state(SwitchButton::OK);

	do_full_update();
}

void CamView::do_full_update() {
	//! @todo need mutex here?
	glarea.link_data((void *) camctrl->monitor.image, camctrl->monitor.depth, camctrl->monitor.x2 - camctrl->monitor.x1, camctrl->monitor.y2 - camctrl->monitor.y1);
	
	// Do histogram, make local copy if needed
	if (camctrl->monitor.histo) {
		if (!histo_img)
			histo_img = (uint32_t *) malloc((1 << camctrl->get_depth()) * sizeof *histo_img);
		memcpy(histo_img, camctrl->monitor.histo, (1 << camctrl->get_depth()) * sizeof *histo_img);
	} 
	else if (histo_img) {
		free(histo_img);
		histo_img = 0;
	}
	
	e_avg.set_text(format("%g", camctrl->monitor.avg));
	e_rms.set_text(format("%g", camctrl->monitor.rms));
	
	do_histo_update();
}

void CamView::on_message_update() {
	//! @bug LOCALE not set properly? problem with , and . in sending values
	DevicePage::on_message_update();

	// Set values in text entries
	e_exposure.set_text(format("%g", camctrl->get_exposure()));
	e_offset.set_text(format("%g", camctrl->get_offset()));
	e_interval.set_text(format("%g", camctrl->get_interval()));
	e_gain.set_text(format("%g", camctrl->get_gain()));
	e_res.set_text(format("%dx%dx%d", camctrl->get_width(), camctrl->get_height(), camctrl->get_depth()));
	
	// Tell glarea what we can expect
	glarea.set_data(camctrl->get_depth(), camctrl->get_width(), camctrl->get_height());
	
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
	
	store_n.set_text(format("%d", camctrl->get_nstore()));
	if (camctrl->get_nstore() == 0)
		store.set_state(SwitchButton::CLEAR);
	else
		store.set_state(SwitchButton::WAITING);

}

void CamView::on_info_change() {
	log.term(format("%s", __PRETTY_FUNCTION__));
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
	// Click the 'capture' button: if running, disable, otherwise start camera
	if (camctrl->get_mode() == CamCtrl::RUNNING || 
			camctrl->get_mode() == CamCtrl::SINGLE) {
        log.term(format("%s Stop cam", __PRETTY_FUNCTION__));
		camctrl->set_mode(CamCtrl::WAITING);
	}
	else {
        log.term(format("%s Start cam", __PRETTY_FUNCTION__));
		camctrl->set_mode(CamCtrl::RUNNING);
	}
}

void CamView::on_display_clicked() {
	// Click the 'display' button: 
	// -> if CLEAR: request frame, set button to WAITING
	// -> if WAITING, OK or ERROR: stop capture 
	if (display.get_state() == SwitchButton::CLEAR) {
 		camctrl->grab(0, 0, camctrl->get_width(), camctrl->get_height(), 1, false, histo.get_active());
		display.set_state(SwitchButton::WAITING);
	}
	else
		display.set_state(SwitchButton::CLEAR);
}

void CamView::on_store_clicked() {
	// Store activated (via button store or entry store_n):
	// - store CLEAR: start storing
	// - store WAITING: store in progress, stop storing
	// - store ERROR: abort
	// - store OK: unused
	
	int nstore = (int) strtol(store_n.get_text().c_str(), NULL, 0);
	log.term(format("%s (%d)", __PRETTY_FUNCTION__, nstore));
	
	if (store.get_state() == SwitchButton::CLEAR) {
		// If the value 'nstore' is valid, 
		if (nstore > 0 || nstore == -1) {
			camctrl->store(nstore);
			store.set_state(SwitchButton::WAITING);
		}
	}
	else
		camctrl->store(0);
	
}

bool CamView::on_histo_clicked(GdkEventButton *event) {
	if (event->type != GDK_BUTTON_PRESS)
		return false;
	
	int shift = camctrl->get_depth() - 8;
	if (shift < 0)
		shift = 0;
	double value = event->x * (1 << shift);
	
	if (event->button == 1)
		minval.set_value(value);
	if (event->button == 2) {
		minval.set_value(value - (16 << shift));
		maxval.set_value(value + (16 << shift));
	}
	if (event->button == 3)
		maxval.set_value(value);
	
	//!< @todo implement histogram binning here
	do_histo_update();
	return true;
}

void CamView::on_minmax_change() {
	glarea.setminmax(minval.get_value_as_int(), maxval.get_value_as_int());
	glarea.do_update();
}

