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
subi_frame("Subimages"),
subi_lx("X_0"), subi_ly("Y_0"), subi_tx("X_1"), subi_ty("Y_1"), 
subi_update("Update"), subi_del("Del"), subi_add("Add"), subi_regen("Regen pattern"), subi_find("Find pattern")
{
	fprintf(stderr, "%x:ShwfsView::ShwfsView()\n", (int) pthread_self());
	
	// Widget properties
	subi_lx.set_width_chars(6);
	subi_ly.set_width_chars(6);
	subi_tx.set_width_chars(6);
	subi_ty.set_width_chars(6);
	
	// Signals & callbacks
	subi_select.signal_changed().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_select_changed));
	subi_add.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_add_clicked));
	subi_del.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_del_clicked));
	subi_update.signal_clicked().connect(sigc::mem_fun(*this, &ShwfsView::on_subi_update_clicked));
	
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
	
	subi_hbox1.pack_start(subi_vbox11, PACK_SHRINK);
	subi_hbox1.pack_start(vsep1, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox12, PACK_SHRINK);
	subi_hbox1.pack_start(vsep2, PACK_SHRINK);
	subi_hbox1.pack_start(subi_vbox13, PACK_SHRINK);
	
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
		
	if (wfscam_ui) {
		wfscam_ui->glarea.clearboxes();
		wfscam_ui->glarea.clearlines();
	}
}

void ShwfsView::on_subi_select_changed() {
	fprintf(stderr, "%x:ShwfsView::on_subi_select_changed()\n", (int) pthread_self());

	// Get current selected item, convert to int
	string tmp = subi_select.get_active_text();
	int curr_si = strtol(tmp.c_str(), NULL, 10);
	
	// Check if selected sub image is within bounds. This also filters out 'Add New'
	if (curr_si < 0 || curr_si >= (int) shwfsctrl->get_mla_nsi())
		return;
	
	// Get coordinates for that subimage
	fvector_t tmp_si = shwfsctrl->get_mla_si((size_t) curr_si);
	
	fprintf(stderr, "%x:ShwfsView::on_subi_select_changed %d, (%.3f, %.3f), (%.3f, %.3f)\n", 
					(int) pthread_self(), curr_si, tmp_si.lx, tmp_si.ly, tmp_si.tx, tmp_si.ty);

	subi_lx.set_text(format("%.0f", tmp_si.lx));
	subi_ly.set_text(format("%.0f", tmp_si.ly));
	subi_tx.set_text(format("%.0f", tmp_si.tx));
	subi_ty.set_text(format("%.0f", tmp_si.ty));
}

void ShwfsView::on_subi_add_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_add_clicked()\n", (int) pthread_self());
	
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
	
	// Add new subimage, delete old one
#error continue here
}

void ShwfsView::on_subi_regen_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_regen_clicked()\n", (int) pthread_self());
	
	// Re-generate subimage pattern
#error continue here
}

void ShwfsView::on_subi_find_clicked() {
	fprintf(stderr, "%x:ShwfsView::on_subi_find_clicked()\n", (int) pthread_self());
	
	// Find subimage pattern heuristically
#error continue here
}

void ShwfsView::do_wfspow_update() {
	WfsView::do_wfspow_update();
	fprintf(stderr, "%x:ShwfsView::do_wfspow_update()\n", (int) pthread_self());
	
	// Add subimage boxes & wavefront vectors to glarea
	if (wfscam_ui) {
		wfscam_ui->glarea.clearboxes();
		
		for (size_t i=0; i<shwfsctrl->get_mla_nsi(); i++) {
			wfscam_ui->glarea.addbox(shwfsctrl->get_mla_si((size_t) i));
		}
		
		//! @todo add drawing vectors for wavefront measurements
	}
	
}

void ShwfsView::do_info_update() {
	WfsView::do_info_update();
	fprintf(stderr, "%x:ShwfsView::do_info_update()\n", (int) pthread_self());
	
	// Add list of subimages
	subi_select.clear_items();
	fprintf(stderr, "%x:ShwfsView::do_info_update(append %zu)\n", (int) pthread_self(), shwfsctrl->get_mla_nsi());
	for (int i=0; i<shwfsctrl->get_mla_nsi(); i++) {
		fprintf(stderr, "%x:ShwfsView::do_info_update(append %d)\n", (int) pthread_self(), i);
		subi_select.append_text(format("%d", i));
	}
	
	// Add text to add a new subimage
	subi_select.append_text("Add new");
}