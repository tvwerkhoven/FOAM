/*
 wfsview.cc -- wavefront sensor viewer class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#include "deviceview.h"
#include "devicectrl.h"
#include "wfsctrl.h"
#include "wfsview.h"

using namespace std;
using namespace Gtk;

WfsView::WfsView(WfsCtrl *wfsctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) wfsctrl, log, foamctrl, n), wfsctrl(wfsctrl),
wfpow_frame("Wavefront info"), 
wfpow_mode("Basis"), wfpow_align(0.5, 0.5, 0, 0)
{
	fprintf(stderr, "%x:WfsView::WfsView()\n", (int) pthread_self());
	
	wfpow_pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, false, 8, 480, 100);
	wfpow_pixbuf->fill(0xFFFFFF00);
	wfpow_img.set(wfpow_pixbuf);
	wfpow_img.set_double_buffered(false);
	
	wfpow_mode.set_width_chars(12);
	wfpow_mode.set_editable(false);
	
	clear_gui();
	disable_gui();
	
	wfpow_events.add(wfpow_img);
	wfpow_align.add(wfpow_events);
	
	wfpow_hbox.pack_start(wfpow_mode, PACK_SHRINK);
	wfpow_hbox.pack_start(wfpow_align);
	wfpow_frame.add(wfpow_hbox);

	pack_start(wfpow_frame, PACK_SHRINK);
	
	wfsctrl->signal_wavefront.connect(sigc::mem_fun(*this, &WfsView::do_wfspow_update));

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
}

void WfsView::do_wfspow_update() {	
	int nmodes = wfsctrl->get_nmodes();
	gsl_vector_float *mode_pow = wfsctrl->get_modes();
	
	if (!mode_pow)
		return;
	
	// Draw the wavefront modes
	
	if (!wfpow_frame.is_visible())
		return;
	
	// mode_pow range is -1 -- 1 for all modes. Check actual maximum:
	double max=0;
	max = gsl_vector_float_max (mode_pow);

	// Color red if (almost) maxed out, make background white otherwise
	if (max > 0.97)
		wfpow_pixbuf->fill(0xff000000);
	else
		wfpow_pixbuf->fill(0xffffff00);
	
	// Draw bars
	uint8_t *out = (uint8_t *)wfpow_pixbuf->get_pixels();
	
	int w = wfpow_pixbuf->get_width();
	int h = wfpow_pixbuf->get_height();
	
	// The pixbuf is w pixels wide, we have nmodes modes, so each column is colw wide:
	int colw = (w/nmodes)-1;
	
	//! @todo this fails for more than ~200 modes, add scrollbar?
	if (colw <= 0)
		return;
	
	for (int n = 0; n < nmodes; n++) {
		int height = gsl_vector_float_get(mode_pow, n)*h/2.0; // should be between -h/2 and h/2
		
		if (height < 0) {
			for (int x = n*colw; x < (n+1)*colw; x++) {
				for (int y = h/2; y > h/2+height; y--) {
					uint8_t *p = out + 3 * (x + w * y);
					p[0] = 0;
					p[1] = 0;
					p[2] = 0;
				}
			}
		}
		else {
			for (int x = n*colw; x < (n+1)*colw; x++) {
				for (int y = h/2; y < h/2+height; y++) {
					uint8_t *p = out + 3 * (x + w * y);
					p[0] = 0;
					p[1] = 0;
					p[2] = 0;
				}
			}				
		}
	}

	wfpow_img.queue_draw();
}

