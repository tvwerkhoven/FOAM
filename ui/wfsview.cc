/*
 wfsview.cc -- wavefront sensor viewer class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include "utils.h"

#include "deviceview.h"
#include "devicectrl.h"
#include "wfsctrl.h"
#include "wfsview.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

WfsView::WfsView(WfsCtrl *wfsctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) wfsctrl, log, foamctrl, n), wfsctrl(wfsctrl),
wf_cam("Cam"), 
wfpow_frame("Wavefront info"), 
wfpow_mode("Basis"), wfpow_align(0.5, 0.5, 0, 0),
wfscam_ui(NULL)
{
	fprintf(stderr, "%x:WfsView::WfsView()\n", (int) pthread_self());
	
	wfpow_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, 480, 100);
	wfpow_pixbuf->fill(0xFFFFFF00);
	wfpow_img.set(wfpow_pixbuf);
	wfpow_img.set_double_buffered(false);
	
	wfpow_mode.set_width_chars(12);
	wfpow_mode.set_editable(false);
	
	wf_cam.set_width_chars(16);
	wf_cam.set_editable(false);
	
	clear_gui();
	disable_gui();
	
	// Extra device info
	devhbox.pack_start(vsep0, PACK_SHRINK);
	devhbox.pack_start(wf_cam, PACK_SHRINK);
		
	// Wavefront power 'spectrum' (separate window)
	wfpow_events.add(wfpow_img);
	wfpow_align.add(wfpow_events);
	
	wfpow_hbox.pack_start(wfpow_mode, PACK_SHRINK);
	wfpow_hbox.pack_start(wfpow_align);
	wfpow_frame.add(wfpow_hbox);
	
	// Extra window
	extra_win.set_title("FOAM WFS " + devname);
	
	extra_vbox.pack_start(wfpow_frame, PACK_SHRINK);
	extra_win.add(extra_vbox);
	
	extra_win.show_all_children();
	extra_win.present();
	
	// Event handlers
	
	wfsctrl->signal_message.connect(sigc::mem_fun(*this, &WfsView::do_info_update));
	wfsctrl->signal_wavefront.connect(sigc::mem_fun(*this, &WfsView::do_wfspow_update));
	wfsctrl->signal_wfscam.connect(sigc::mem_fun(*this, &WfsView::do_cam_update));
	
	// finalize
	show_all_children();
}

WfsView::~WfsView() {
	fprintf(stderr, "%x:WfsView::~WfsView()\n", (int) pthread_self());
}

void WfsView::enable_gui() {
	DevicePage::enable_gui();
	fprintf(stderr, "%x:WfsView::enable_gui()\n", (int) pthread_self());

}

void WfsView::disable_gui() {
	DevicePage::disable_gui();
	fprintf(stderr, "%x:WfsView::disable_gui()\n", (int) pthread_self());
	
}

void WfsView::clear_gui() {
	DevicePage::clear_gui();
	fprintf(stderr, "%x:WfsView::clear_gui()\n", (int) pthread_self());
	
	wf_cam.set_text("N/A");
	
	wfpow_mode.set_text("N/A");
}

void WfsView::do_info_update() {
	// Set wavefront basis text
	wfpow_mode.set_text(wfsctrl->get_basis());
	
	if (wfscam_ui)
		wf_cam.set_text(wfsctrl->wfscam);
}
	
void WfsView::do_wfspow_update() {	
	int nmodes = wfsctrl->get_nmodes();
	
	// Return if nothing to be drawn
	if (!wfpow_frame.is_visible() || nmodes <= 0)
		return;
	
	// mode_pow range is -1 -- 1 for all modes. Check actual maximum:
	double max=0;
	for (int m = 0; m < nmodes; m++)
		if (wfsctrl->get_mode(m) > max)
			max = wfsctrl->get_mode(m);
	
	// make background white
	wfpow_pixbuf->fill(0xffffff00);
	
	// Draw bars
	uint8_t *out = (uint8_t *) wfpow_pixbuf->get_pixels();
	
	int w = wfpow_pixbuf->get_width();
	int h = wfpow_pixbuf->get_height();
	
	// The pixbuf is w pixels wide, we have nmodes modes, so each column is colw wide:
	int colw = (w/nmodes)-1;
	
	//! @bug this fails for more than ~200 modes, add scrollbar?
	if (colw <= 0) {
		fprintf(stderr, "WfsView::do_wfspow_update(): error, too many wavefronts, cannot draw!\n");
		return;
	}
	
	uint8_t col[3];
	for (int n = 0; n < nmodes; n++) {
		float amp = clamp(wfsctrl->get_mode(n), -1.0f, 1.0f);
		int height = amp*h/2.0; // should be between -h/2 and h/2
		
		// Set bar color (red 2%, orange 10% or green rest):
		if (fabs(amp)>0.98) {
			col[0] = 255; col[1] = 000; col[2] = 000; // X11 red
		} else if (fabs(amp)>0.90) {
			col[0] = 255; col[1] = 165; col[2] = 000; // X11 orange
		} else {
			col[0] = 144; col[1] = 238; col[2] = 144; // X11 lightgreen
		}
		
		// Bars going down (first part) or up (second part)
		if (height < 0) {
			for (int x = n*colw; x < (n+1)*colw; x++) {
				for (int y = h/2; y > h/2+height; y--) {
					uint8_t *p = out + 3 * (x + w * y);
					p[0] = col[0]; p[1] = col[1]; p[2] = col[2];
		} } } else {
			for (int x = n*colw; x < (n+1)*colw; x++) {
				for (int y = h/2; y < h/2+height; y++) {
					uint8_t *p = out + 3 * (x + w * y);
					p[0] = col[0]; p[1] = col[1]; p[2] = col[2];
		} } }
	}

	wfpow_img.queue_draw();
}

void WfsView::do_cam_update() {
	// New camera for this WFS, get from foamctrl and store in this class.
	FoamControl::device_t *dev_wfscam = foamctrl.get_device(wfsctrl->wfscam);
	wfscam_ui = (CamView *) dev_wfscam->page;
	
	// update gui with new information
	do_info_update();
	do_wfspow_update();
}


