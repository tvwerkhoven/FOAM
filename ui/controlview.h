/*
 controlview.h -- FOAM GUI connection control pane
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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
/*!
 @file controlview.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 @brief This is the FOAM connection control pane
 */

#ifndef HAVE_CONTROLVIEW_H
#define HAVE_CONTROLVIEW_H

#include <gtkmm.h>

#include "widgets.h"
#include "log.h"
#include "foamcontrol.h"

using namespace Gtk;
using namespace std;

/*!
 @brief GUI for base connection to FOAM
 
 This is the GUI for the underlying foamctrl, which handles connections to
 FOAM.
 */
class ControlPage: public VBox {
	Log &log;
	FoamControl &foamctrl;
	
	Frame connframe;
	HBox connbox;
	LabeledEntry host;
	LabeledEntry port;
	Button connect;											//!< Init connection, ControlPage::on_connect_clicked()
	
	HBox modecalibbox;
	Frame modeframe;
	HBox modebox;
	SwitchButton mode_listen;						//!< Switch FOAM mode to listen, ControlPage::on_mode_listen_clicked()
	SwitchButton mode_open;							//!< Switch FOAM mode to open, ControlPage::on_mode_open_clicked()
	SwitchButton mode_closed;						//!< Switch FOAM mode to closed, ControlPage::on_mode_closed_clicked()
	Button shutdown;										//!< Shutdown FOAM, ControlPage::on_shutdown_clicked()

	Frame calibframe;
	HBox calibbox;
	Label calmode_lbl;
	ComboBoxText calmode_select;				//!< Possible calibrations modes in FOAM
	SwitchButton calib;									//!< Start FOAM calibration mode, ControlPage::on_calib_clicked()

	Frame statframe;
	HBox statbox;
	LabeledEntry stat_mode;							//!< Displays current FOAM mode, updated in ControlPage::on_message_update()
	LabeledEntry stat_ndev;							//!< Displays number of FOAM devices, updated in ControlPage::on_message_update()
	LabeledEntry stat_nframes;					//!< Displays number of frames captured, updated in ControlPage::on_message_update()
	LabeledEntry stat_lastcmd;					//!< Displays last cmd to FOAM, updated in ControlPage::on_message_update()

	
	Frame devframe;											//!< @todo add/improve this feature
	HBox devbox;
	LabeledEntry *dev_devlist;
	
	void on_connect_clicked();					//!< Callback for ControlPage::connect
	
	void on_mode_listen_clicked();			//!< Callback for ControlPage::mode_listen
	void on_mode_open_clicked();				//!< Callback for ControlPage::mode_open
	void on_mode_closed_clicked();			//!< Callback for ControlPage::mode_closed
	void on_shutdown_clicked();					//!< Callback for ControlPage::shutdown

	void on_calib_clicked();						//!< Callback for ControlPage::calib

	void enable_gui();									//!< Enable GUI elements
	void disable_gui();									//!< Disable GUI elements
	void clear_gui();										//!< Clear GUI elements (reset values)

	
public:
	ControlPage(Log &log, FoamControl &foamctrl);
	~ControlPage();

	void on_connect_update();						//!< Callback for FoamCtrl::signal_connect()
	void on_message_update();						//!< Callback for FoamCtrl::signal_message()
};

#endif //  HAVE_CONTROLVIEW_H
