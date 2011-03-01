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
protected:
	ShwfsCtrl *shwfsctrl;
	
	// MLA / subimage controls
	Frame subi_frame;										//!< Frame for subimage controls
	HBox subi_hbox1;
	VBox subi_vbox11;
	HBox subi_hbox111;
	VBox subi_vbox12;
	HBox subi_hbox121;
	HBox subi_hbox122;
	VBox subi_vbox13;
	VSeparator vsep1;
	VSeparator vsep2;
	
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

	// From DeviceView::
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();

	// From WfsView::
	virtual void do_wfspow_update();
	virtual void do_info_update();
	
	// Extra events:
	void on_subi_select_changed();
	
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
