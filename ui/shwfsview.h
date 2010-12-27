/*
 shwfsview.h -- wavefront sensor control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
	
	// From WfsView::
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();
	
public:
	ShwfsView(ShwfsCtrl *ctrl, Log &log, FoamControl &foamctrl, string n);
	~ShwfsView();
};


#endif // HAVE_SHWFSVIEW_H


/*!
 \page dev_wfs Wavefront sensor devices : ShwfsView & WfsCtrl

 */
