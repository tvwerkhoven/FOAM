/*
 telescopeview.h -- Telescope GUI
 Copyright (C) 2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#ifndef HAVE_TELESCOPEVIEW_H
#define HAVE_TELESCOPEVIEW_H

#include <gtkmm.h>
#include <gdkmm/pixbuf.h>
#include <gtkglmm.h>

#include "widgets.h"

#include "telescopectrl.h"
#include "deviceview.h"

/*!
 @brief Generic telescope GUI class
 
 */
class TelescopeView: public DevicePage {
private:
	TelescopeCtrl *telescopectrl;
	

	// From DevicePage::
	virtual void on_message_update();
	
	virtual void enable_gui();
	virtual void disable_gui();
	virtual void clear_gui();
	
public:
	TelescopeView(TelescopeCtrl *telescopectrl, Log &log, FoamControl &foamctrl, string n);
	~TelescopeView();
};


#endif // HAVE_TELESCOPEVIEW_H

/*!
 \page dev_telescope_ui Telescope UI : TelescopeView & TelescopeCtrl

 \section telescopeview_telescopeview TelescopeView
 
 Shows a basic GUI for a generic telescope. See TelescopeView
 
 \section telescopeview_telescopectrl TelescopeCtrl
 
 Controls a generic telescope. See TelescopeCtrl.
 
 \section telescopeview_derived Derived classes
 
 The following classes are derived from this class:
 - none
 
 \section telescopeview_related Related classes
 
 The following classes are closely related to this class:
 - none
 
 
 */
