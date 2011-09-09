/*
 shwfsview.h -- wavefront sensor control class
 Copyright (C) 2010--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,ShwfsView
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HAVE_SHWFSVIEW_H
#define HAVE_SHWFSVIEW_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "shwfsctrl.h"
#include "wfsview.h"

/*!
 @brief Shack-Hartmann wavefront sensor GUI class
 
 */
class ShwfsView: public WfsView {
private:
	ShwfsCtrl *shwfsctrl;
	
	const string shwfs_addnew;
	
	// MLA / subimage controls
	Frame subi_frame;										//!< Frame for subimage controls
	HBox subi_hbox1;
	VBox subi_vbox11;
	HBox subi_hbox111;
	VSeparator vsep1;
	VBox subi_vbox12;
	HBox subi_hbox121;
	HBox subi_hbox122;
	VSeparator vsep2;
	VBox subi_vbox13;
	VSeparator vsep3;
	VBox subi_vbox14;
	HBox subi_hbox141;
	
	ComboBoxText subi_select;						//!< List of subimages
	LabeledEntry subi_lx;								//!< Subimage coordinate lower x
	LabeledEntry subi_ly;								//!< Subimage coordinate lower y
	LabeledEntry subi_tx;								//!< Subimage coordinate top x
	LabeledEntry subi_ty;								//!< Subimage coordinate top y
	
	Button subi_update;									//!< Update current subimage data
	Button subi_del;										//!< Delete current subimage
	Button subi_add;										//!< Add new subimage
	
	Button subi_regen;									//!< Re-generate subimage pattern
	Button subi_find;										//!< Find subimage pattern
	
	SwitchButton subi_vecs;							//!< Display shift vectors or not
																			//! @todo convert subi_vecdelayi to spin entry
	LabeledEntry subi_vecdelayi;				//!< Display shift vectors every X seconds
	float subi_vecdelay;								//!< Delay for displaying shift vectors (in seconds)

	// From DeviceView::
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();

	virtual void on_message_update();
	
	// New:
	void do_sh_shifts_update();					//!< Connected to ShwfsCtrl::signal_sh_shifts(), fires when new SHWFS shifts are available
	
	// Extra events:
	void on_subi_select_changed();			//!< Select a subimage combobox
	void on_subi_add_clicked();					//!< Add new subimage button
	void on_subi_del_clicked();					//!< Delete selected subimage button
	void on_subi_update_clicked();			//!< Update current subimage coordinates
	
	void on_subi_regen_clicked();				//!< Re-generate new MLA pattern
	void on_subi_find_clicked();				//!< Heuristically find new MLA pattern
	
	void on_subi_vecs_clicked();				//!< Show SHWFS shift vectors toggler
	
	bool on_timeout();
	
public:
	ShwfsView(ShwfsCtrl *ctrl, Log &log, FoamControl &foamctrl, string n);
	~ShwfsView();
};


#endif // HAVE_SHWFSVIEW_H


/*!
 \page dev_wfs_shwfs_ui Shack-Hartmann Wavefront sensor devices UI : ShwfsView & ShwfsCtrl
 
 \section shwfsview_shwfsview ShwfsView
 
 Shows a basic GUI for a Shack-Hartmann wavefront sensor. See ShwfsView
 
 \section shwfsview_shwfsctrl ShwfsCtrl
 
 Controls a Shack-Hartmann wavefront sensor. See ShwfsCtrl.
 
 */
