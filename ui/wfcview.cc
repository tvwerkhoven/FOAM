/*
 wfcview.cc -- wavefront corrector viewer class
 Copyright (C) 2010--2011 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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
#include "wfcctrl.h"
#include "wfcview.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

WfcView::WfcView(WfcCtrl *wfcctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) wfcctrl, log, foamctrl, n), 
wfcctrl(wfcctrl),
wfc_nact("#Act."),
calib_frame("Calibration"), calib_setall("Set all to"), calib_setactid("Set act #"), calib_setactval("to"),
calib_random("Set Random"), calib_waffle("Set Waffle"), calib_amp("amp."),
ctrl_frame("Control"), ctrl_gainp("Gain", "P", -INFINITY), ctrl_gaini("", "I", -INFINITY), ctrl_gaind("", "D", -INFINITY),
wfcact_frame("WFC actuators"), wfcact_gr(480,100)
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	wfc_nact.set_width_chars(8);
	wfc_nact.set_editable(false);
	
	calib_setall.set_digits(2);
	calib_setall.set_increments(0.1, 1);
	calib_setall.set_range(-1.0,1.0);

	calib_setactid.set_digits(0);
	calib_setactid.set_increments(1, 10);
	
	calib_setactval.set_digits(2);
	calib_setactval.set_increments(0.1, 1);
	calib_setactval.set_range(-1.0,1.0);
	
	calib_amp.set_digits(2);
	calib_amp.set_increments(0.1, 1);
	calib_amp.set_range(-5.0, 5.0);

	ctrl_gainp.set_digits(2);
	ctrl_gainp.set_increments(0.1, 1);
	ctrl_gaini.set_digits(2);
	ctrl_gaini.set_increments(0.1, 1);
	ctrl_gaind.set_digits(2);
	ctrl_gaind.set_increments(0.1, 1);

	// Extra device info
	devhbox.pack_start(vsep0, PACK_SHRINK);
	devhbox.pack_start(wfc_nact, PACK_SHRINK);
	
	// Calib frame
	calib_hbox.pack_start(calib_setall, PACK_SHRINK);
	calib_hbox.pack_start(vsep1, PACK_SHRINK);
	calib_hbox.pack_start(calib_setactid, PACK_SHRINK);
	calib_hbox.pack_start(calib_setactval, PACK_SHRINK);
	calib_hbox.pack_start(vsep2, PACK_SHRINK);
	calib_hbox.pack_start(calib_random, PACK_SHRINK);
	calib_hbox.pack_start(calib_waffle, PACK_SHRINK);
	calib_hbox.pack_start(calib_amp, PACK_SHRINK);
	calib_frame.add(calib_hbox);
	
	// Ctrl frame
	ctrl_hbox.pack_start(ctrl_gainp, PACK_SHRINK);
	ctrl_hbox.pack_start(ctrl_gaini, PACK_SHRINK);
	ctrl_hbox.pack_start(ctrl_gaind, PACK_SHRINK);
	ctrl_frame.add(ctrl_hbox);
	
	// Add to main GUI page
	pack_start(calib_frame, PACK_SHRINK);
	pack_start(ctrl_frame, PACK_SHRINK);

	// Wavefront corrector actuator 'spectrum' (separate window)
	wfcact_hbox.pack_start(wfcact_gr);
	wfcact_frame.add(wfcact_hbox);
	
	// Extra window
	extra_win.set_title("FOAM WFC " + devname);
	
	extra_win.set_default_size(640, 140);

	extra_vbox.pack_start(wfcact_frame, PACK_SHRINK);
	extra_win.add(extra_vbox);
	
	extra_win.show_all_children();
	extra_win.present();
	
	// wfcact_gr needs to know about the function to get updated values
	wfcact_gr.slot_update = sigc::mem_fun(*this, &WfcView::do_wfcact_update);
	
	// Event handlers
	wfcctrl->signal_wfcctrl.connect(sigc::mem_fun(*this, &WfcView::on_wfcact_update));
	wfcctrl->signal_message.connect(sigc::mem_fun(*this, &WfcView::on_message_update));
	
	calib_random.signal_clicked().connect(sigc::mem_fun(*this, &WfcView::on_calib_random_clicked));
	calib_waffle.signal_clicked().connect(sigc::mem_fun(*this, &WfcView::on_calib_waffle_clicked));
	calib_setall.entry.signal_activate().connect(sigc::mem_fun(*this, &WfcView::on_calib_setall_act));
	calib_setactid.entry.signal_activate().connect(sigc::mem_fun(*this, &WfcView::on_calib_setact_act));
	calib_setactval.entry.signal_activate().connect(sigc::mem_fun(*this, &WfcView::on_calib_setact_act));

	ctrl_gainp.entry.signal_activate().connect(sigc::mem_fun(*this, &WfcView::on_gain_act));
	ctrl_gaini.entry.signal_activate().connect(sigc::mem_fun(*this, &WfcView::on_gain_act));
	ctrl_gaind.entry.signal_activate().connect(sigc::mem_fun(*this, &WfcView::on_gain_act));

	clear_gui();
	disable_gui();
	
	// finalize
	show_all_children();
}

WfcView::~WfcView() {
	log.term(format("%s", __PRETTY_FUNCTION__));
}

void WfcView::enable_gui() {
	DevicePage::enable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	calib_setall.set_sensitive(true);
	calib_setactid.set_sensitive(true);
	calib_setactval.set_sensitive(true);
	calib_random.set_sensitive(true);
	calib_waffle.set_sensitive(true);
	calib_amp.set_sensitive(true);
	ctrl_gainp.set_sensitive(true);
	ctrl_gaini.set_sensitive(true);
	ctrl_gaind.set_sensitive(true);
}

void WfcView::disable_gui() {
	DevicePage::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	calib_setall.set_sensitive(false);
	calib_setactid.set_sensitive(false);
	calib_setactval.set_sensitive(false);
	calib_random.set_sensitive(false);
	calib_waffle.set_sensitive(false);
	calib_amp.set_sensitive(false);
	ctrl_gainp.set_sensitive(false);
	ctrl_gaini.set_sensitive(false);
	ctrl_gaind.set_sensitive(false);
}

void WfcView::clear_gui() {
	DevicePage::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	calib_setall.set_value(0);
	calib_setactid.set_value(0);
	calib_setactval.set_value(0);
	calib_amp.set_value(0.1);
	ctrl_gainp.set_value(0.1);
	ctrl_gaini.set_value(0.0);
	ctrl_gaind.set_value(0.0);
}

void WfcView::on_wfcact_update() {
	// Return if nothing to be drawn
	if (!wfcact_frame.is_visible())
		return;

	wfcact_gr.on_update(wfcctrl->get_ctrlvec());
}

void WfcView::on_calib_random_clicked() {
	wfcctrl->send_cmd(format("act random %g", calib_amp.get_value()));
}

void WfcView::on_calib_waffle_clicked() {
	wfcctrl->send_cmd(format("act waffle %g", calib_amp.get_value()));
}

void WfcView::on_calib_setall_act() {
	wfcctrl->send_cmd(format("act all %g", calib_setall.get_value()));
}

void WfcView::on_calib_setact_act() {
	double actval = calib_setactval.get_value();
	int actid = calib_setactid.get_value_as_int();
	wfcctrl->send_cmd(format("act one %d %g", actid, actval));
}

void WfcView::on_gain_act() {
	wfcctrl->send_cmd(format("set gain %g %g %g", 
													 ctrl_gainp.get_value(),
													 ctrl_gaini.get_value(),
													 ctrl_gaind.get_value()));
}

void WfcView::on_message_update() {
	DevicePage::on_message_update();
	
	calib_setactid.set_range(0, wfcctrl->get_nact()-1);
	wfc_nact.set_text(format("%d", wfcctrl->get_nact()));
	
	// Update gain setting
	ctrl_gainp.set_value(wfcctrl->get_gain().p);
	ctrl_gaini.set_value(wfcctrl->get_gain().i);
	ctrl_gaind.set_value(wfcctrl->get_gain().d);
}
