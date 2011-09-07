/*
 wfcview.cc -- wavefront corrector viewer class
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
#include "wfcctrl.h"
#include "wfcview.h"
#include "camview.h"

using namespace std;
using namespace Gtk;

WfcView::WfcView(WfcCtrl *wfcctrl, Log &log, FoamControl &foamctrl, string n): 
DevicePage((DeviceCtrl *) wfcctrl, log, foamctrl, n), 
wfcctrl(wfcctrl),
wfc_nact("#Act."),
calib_frame("Calibration"), calib_setall("Set all"), calib_setactid("Set act #"), calib_setactval("to"),
calib_random("Set Random"), calib_waffle("Set Waffle"),
wfcact_frame("WFC actuators"), wfcact_gr(480,100)
{
	log.term(format("%s", __PRETTY_FUNCTION__));
	
	wfc_nact.set_width_chars(8);
	wfc_nact.set_editable(false);
	
	calib_setall.set_width_chars(4);

	calib_setactid.set_digits(0);
	calib_setactid.set_increments(1, 10);
	
	calib_setactval.set_width_chars(4);

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
	calib_frame.add(calib_hbox);
		
	// Wavefront corrector actuator 'spectrum' (separate window)
	wfcact_hbox.pack_start(wfcact_gr);
	wfcact_frame.add(wfcact_hbox);
	
	pack_start(wfcact_frame, PACK_SHRINK);
	
	// Extra window
	extra_win.set_title("FOAM WFC " + devname);
	
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
}

void WfcView::disable_gui() {
	DevicePage::disable_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	calib_setall.set_sensitive(false);
	calib_setactid.set_sensitive(false);
	calib_setactval.set_sensitive(false);
	calib_random.set_sensitive(false);
	calib_waffle.set_sensitive(false);
}

void WfcView::clear_gui() {
	DevicePage::clear_gui();
	log.term(format("%s", __PRETTY_FUNCTION__));
	calib_setall.set_text("0");
	calib_setactid.set_value(0);
	calib_setactval.set_text("0");
}

void WfcView::on_wfcact_update() {
	// Return if nothing to be drawn
	if (!wfcact_frame.is_visible())
		return;

	wfcact_gr.on_update(wfcctrl->get_ctrlvec());
}

void WfcView::on_calib_random_clicked() {
	//wfcctrl->act_random();
}

void WfcView::on_calib_waffle_clicked() {
	//wfcctrl->act_waffle();
}

void WfcView::on_calib_setall_act() {
	double actval = strtof(calib_setall.get_text().c_str(), NULL);
	//wfcctrl->act_all(actval);
}

void WfcView::on_calib_setact_act() {
	double actval = strtof(calib_setactval.get_text().c_str(), NULL);
	int actid = calib_setactid.get_value_as_int();
	//wfcctrl->act_one(actid, actval);
}

void WfcView::on_message_update() {
	DevicePage::on_message_update();
	
	calib_setactid.set_range(0, wfcctrl->get_nact()-1);
	wfc_nact.set_text(format("%d", wfcctrl->get_nact()));
}
