/*
 shwfsview.cc -- shack-hartmann wavefront sensor viewer class
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

#include "types.h"

#include "wfsview.h"
#include "shwfsctrl.h"
#include "shwfsview.h"

using namespace std;
using namespace Gtk;

ShwfsView::ShwfsView(ShwfsCtrl *ctrl, Log &log, FoamControl &foamctrl, string n): 
WfsView((WfsCtrl *) ctrl, log, foamctrl, n), shwfsctrl(ctrl),
shwfs_addnew("Add new"),
subi_frame("Subimages"),
subi_lx("X_0"), subi_ly("Y_0"), subi_tx("X_1"), subi_ty("Y_1"), 
subi_update("Update"), subi_del("Del"), subi_add("Add"), subi_regen("Regen pattern"), subi_find("Find pattern"), 
subi_vecs("Show shifts"), subi_vecdelayi("Delay", "s")
{
	fprintf(stderr, "%x:ShwfsView::ShwfsView()\n", (int) pthread_self());
	
	// Widget properties
	subi_lx.set_width_chars(6);
	subi_ly.set_width_chars(6);
	subi_tx.set_width_chars(6);
	subi_ty.set_width_chars(6);
	subi_vecdelayi.set_width_chars(4);
	
	// Signals & callbacks
	subi_select.signal_changed().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_select_changed));
	subi_add.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_add_clicked));
	subi_del.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_del_clicked));
	subi_update.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_update_clicked));
	
	subi_regen.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_regen_clicked));
	subi_find.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_find_clicked));
	
	subi_vecs.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_vecs_clicked));
	//subi_vecdelayi.entry.signal_activate().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_vecs_clicked));
	
	shwfsctrl->signal_sh_shifts.connect(sigc::mem_fun(*this, &ShwfsView::do_sh_shifts_update));
	Glib::signal_timeout().connect(sigc::mem_fun(*this, &ShwfsView::on_timeout), 1000.0/30.0);
	
	// Add widgets
	// Subimage/ MLA pattern controls
	subi_hbox1.set_spacing(4);
	
	subi_hbox111.pack_start(subi_add, PACK_SHRINK);
	subi_hbox111.pack_start(subi_del, PACK_SHRINK);
	subi_hbox111.pack_start(subi_update, PACK_SHRINK);
	subi_vbox11.pack_start(subi_select, PACK_SHRINK);
	subi_vbox11.pack_start(subi_hbox111, PACK_SHRINK);
	
	subi_hbox121.pack_start(subi_lx, PACK_SHRINK);
	subi_hbox121.pack_start(subi_ly, PACK_SHRINK);
	subi_hbox122.pack_start(subi_tx, PACK_SHRINK);
	subi_hbox122.pack_start(subi_ty, PACK_SHRINK);
	subi_vbox12.pack_start(subi_hbox121 ,PACK_SHRINK);
	subi_vbox12.pack_start(subi_hbox122 ,PACK_SHRINK);
	
	subi_vbox13.pack_start(subi_regen, PACK_SHRINK);
	subi_vbox13.pack_start(subi_find, PACK_SHRINK);
	
	subi_hbox141.pack_start(subi_vecs, PACK_SHRINK);
	subi_hbox141.pack_start(subi_vecdelayi, PACK_SHRINK);
	subi_vbox14.pack_start(subi_hbox141, PACK_SHRINK);
	
	subi_hbox1.pack_start(subi_vbox11, PACK_SHRINK);
	subi_hbox1.pack_start(vsep1, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox12, PACK_SHRINK);
	subi_hbox1.pack_start(vsep2, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox13, PACK_SHRINK);
	subi_hbox1.pack_start(vsep3, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox14, PACK_SHRINK);

	subi_frame.add(subi_hbox1);
	
	pack_start(subi_frame, PACK_SHRINK);
	
	clear_gui();
	disable_gui();
	
	// finalize
	show_all_children();
}

ShwfsView::~ShwfsView() {
	fprintf(stderr, "%x:ShwfsView::~ShwfsView()\n", (int) pthread_self());
}

void ShwfsView::enable_gui() {
	WfsView::enable_gui();
	fprintf(stderr, "%x:ShwfsView::enable_gui()\n", (int) pthread_self());
	
	subi_select.set_sensitive(true);
	subi_update.set_sensitive(true);
	subi_del.set_sensitive(true);
	subi_add.set_sensitive(true);
	subi_regen.set_sensitive(true);
	subi_find.set_sensitive(true);
	subi_vecs.set_sensitive(true);
}

void ShwfsView::disable_gui() {
	WfsView::disable_gui();
	fprintf(stderr, "%x:ShwfsView::disable_gui()\n", (int) pthread_self());
	
	subi_select.set_sensitive(false);
	subi_update.set_sensitive(false);
	subi_del.set_sensitive(false);
	subi_add.set_sensitive(false);
	subi_regen.set_sensitive(false);
	subi_find.set_sensitive(false);
	subi_vecs.set_sensitive(false);	
}

void ShwfsView::clear_gui() {
	WfsView::clear_gui();
	fprintf(stderr, "%x:ShwfsView::clear_gui()\n", (int) pthread_self());
	
	subi_select.clear_items();
	subi_select.append_text("-");
	subi_lx.set_text("");
	subi_ly.set_text("");
	subi_tx.set_text("");
	subi_ty.set_text("");
	subi_vecdelayi.set_text("1.0");
		
	if (wfscam_ui) {
		wfscam_ui->glarea.clearboxes();
		wfscam_ui->glarea.clearlines();
	}
	
	subi_vecs.set_state(SwitchButton::CLEAR);
}

void ShwfsView::on_subi_select_changed() {
	// Get current selected item, convert to int
	string tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Get coordinates for that subimage
	fvector_t tmp_si = shwfsctrl->get_mla_si((size_t) curr_si);
	
	subi_lx.set_text(format("%.0f", tmp_si.lx));
	subi_ly.set_text(format("%.0f", tmp_si.ly));
	subi_tx.set_text(format("%.0f", tmp_si.tx));
	subi_ty.set_text(format("%.0f", tmp_si.ty));
}

void ShwfsView::on_subi_add_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_add_clicked()\n", (int) pthread_self());
	
	string tmp = subi_select.get_active_text();
	if (tmp != shwfs_addnew) {
		fprintf(stderr, "%x:ShwfsView::on_subi_add_clicked() Select '%s' first to add new subimages\n", (int) pthread_self(), shwfs_addnew.c_str());
		return;
	}

	// Get text from GUI, convert to floats
	// Read as floats to process potential decimal signals, then cast to int. 
	// Reading as ints directly might break on decimals.
	tmp = subi_lx.get_text();
	int new_lx = (int) strtof(tmp.c_str(), NULL);
	tmp = subi_ly.get_text();
	int new_ly = (int) strtof(tmp.c_str(), NULL);
	tmp = subi_tx.get_text();
	int new_tx = (int) strtof(tmp.c_str(), NULL);
	tmp = subi_ty.get_text();
	int new_ty = (int) strtof(tmp.c_str(), NULL);
	
	// Call shwfsctrl to add this subimage to the MLA grid
	shwfsctrl->mla_add_si(new_lx, new_ly, new_tx, new_ty);
}

void ShwfsView::on_subi_del_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_del_clicked()\n", (int) pthread_self());
	
	// Get current selected item, convert to int
	string tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Call shwfsctrl to remove this subimage
	shwfsctrl->mla_del_si(curr_si);
}

void ShwfsView::on_subi_update_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_update_clicked()\n", (int) pthread_self());
	
	string tmp;
	
	// Get text from GUI, convert to floats
	// Read as floats to process potential decimal signals, then cast to int. 
	// Reading as ints directly might break on decimals.
	tmp = subi_lx.get_text();
	int new_lx = (int) strtof(tmp.c_str(), NULL);
	tmp = subi_ly.get_text();
	int new_ly = (int) strtof(tmp.c_str(), NULL);
	tmp = subi_tx.get_text();
	int new_tx = (int) strtof(tmp.c_str(), NULL);
	tmp = subi_ty.get_text();
	int new_ty = (int) strtof(tmp.c_str(), NULL);
	
	// Get current selected item, convert to int
	tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Add new subimage, delete old one
	shwfsctrl->mla_update_si(curr_si, new_lx, new_ly, new_tx, new_ty);
}

void ShwfsView::on_subi_regen_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_regen_clicked()\n", (int) pthread_self());
	
	// Re-generate subimage pattern
	shwfsctrl->mla_regen_pattern();
}

void ShwfsView::on_subi_find_clicked() {
	//! @todo Implement mla find with parameters: mla find [sisize] [simini_f] [nmax] [iter]
	fprintf(stderr, "%x:ShwfsView::on_subi_find_clicked()\n", (int) pthread_self());
	
	// Find subimage pattern heuristically
	shwfsctrl->mla_find_pattern();
}

void ShwfsView::on_subi_vecs_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_vecs_clicked()\n", (int) pthread_self());
	
	// Get (new) update delay
	//! @todo is this legal/safe? subi_vecdelayi.get_text().c_str()
	subi_vecdelay = strtof(subi_vecdelayi.get_text().c_str(), NULL);
	
	// If the button is clear: start acquisition, otherwise: set button to clear (and stop acquisition)
	if (subi_vecs.get_state() == SwitchButton::CLEAR) {
		shwfsctrl->cmd_get_shifts();
		subi_vecs.set_state(SwitchButton::WAITING);
	}
	else {
		subi_vecs.set_state(SwitchButton::CLEAR);
	}
}

bool ShwfsView::on_timeout() {
	// This function fires 30 times/s, if we do not want that many updates we can throttle it with subi_vecdelay
	static struct timeval now, last;
	gettimeofday(&now, NULL);
	// If button is 'clear' we don't want shifts, if 'waiting' we're expecting new shifts soon, otherwise if button is 'OK': get new shifts
	if (subi_vecs.get_state() == SwitchButton::OK && 
			((now.tv_sec - last.tv_sec) + (now.tv_usec - last.tv_usec)/1000000.) > subi_vecdelay) {
		subi_vecs.set_state(SwitchButton::WAITING);
		shwfsctrl->cmd_get_shifts();
		gettimeofday(&last, NULL);
	}
	return true;
}

void ShwfsView::do_sh_shifts_update() {
	fprintf(stderr, "%x:ShwfsView::do_sh_shifts_update()\n", (int) pthread_self());
	subi_vecs.set_state(SwitchButton::OK);
		
	// Add image shift vectors to glarea
	if (wfscam_ui) {
		wfscam_ui->glarea.clearlines();
		
		for (size_t i=0; i<shwfsctrl->get_nshifts(); i++) {
			fvector_t tmp = shwfsctrl->get_shift((size_t) i);
			// Add 0.5 to all coordinates because we want the arrow to begin in the middle of the pixel
			tmp.add(0.5, 0.5, 0.5, 0.5);
			wfscam_ui->glarea.addline(tmp);
		}
		
		wfscam_ui->glarea.do_update();
	}
}

void ShwfsView::do_info_update() {
	WfsView::do_info_update();
	fprintf(stderr, "%x:ShwfsView::do_info_update()\n", (int) pthread_self());

	// Add list of subimages to dropdown box
	subi_select.clear_items();
	for (size_t i=0; i<shwfsctrl->get_mla_nsi(); i++)
		subi_select.append_text(format("%d", (int) i));
	
	// Add text to add a new subimage
	subi_select.append_text(shwfs_addnew);
	
	// Add subimage boxes & wavefront vectors to glarea
	if (wfscam_ui) {
		wfscam_ui->glarea.clearboxes();
		
		for (size_t i=0; i<shwfsctrl->get_mla_nsi(); i++) {
			wfscam_ui->glarea.addbox(shwfsctrl->get_mla_si((size_t) i));
		}
	
		wfscam_ui->glarea.do_update();
	}	
}