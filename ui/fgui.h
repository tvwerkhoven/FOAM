/*
 fgui.h -- the FOAM GUI header file
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

#ifndef HAVE_FGUI_H
#define HAVE_FGUI_H

#include <iostream>
#include <string>
#include <map>

#include <gtkmm.h>

#include "about.h"
#include "widgets.h"
#include "log.h"
#include "logview.h"
#include "foamcontrol.h"
#include "controlview.h"
#include "deviceview.h"

using namespace Gtk;
using namespace std;

/*!
 @brief Pop-up dialog for connecting
 */
class ConnectDialog: public Dialog {
	FoamControl &foamctrl;
	
	Label label;
	LabeledEntry host;
	LabeledEntry port;
	
	void on_ok_clicked();
	void on_cancel_clicked();
public:
	ConnectDialog(FoamControl &foamctrl);
	~ConnectDialog() {}
};

/*!
 @brief Menu for MainWindow window
 */
class MainMenu: public MenuBar {
	MenuItem file;
	MenuItem help;
	
	Menu filemenu;
	Menu helpmenu;
	
	SeparatorMenuItem sep1;
	SeparatorMenuItem sep2;
	
public:
	ImageMenuItem connect;							//!< Show ConnectDialog to connect
	ImageMenuItem quit;									//!< Quit GUI
	
	ImageMenuItem about;								//!< Show AboutFOAMGui
	
	MainMenu(Window &window);
	~MainMenu() {};
};

/*!
 @brief Main window used in the GUI. Everything starts here.
 
 This is the main window of the FOAM GUI. It starts all other GUI and non-GUI 
 components.
 
 The most important part are the member foamctrl & controlpage which take care
 of base connections to FOAM. When required, extra GUI components as tabs of
 'notebook' are added with their own control connection.
 */
class MainWindow: public Window {
private:
	VBox vbox;
	
	Log log;														//!< This logs messages (see LogPage and Log)
	FoamControl foamctrl;								//!< This is the base connection to FOAM

	AboutFOAMGui aboutdialog;						//!< About dialog
	Notebook notebook;									//<! Notebook contains the different control tabs
	ConnectDialog conndialog;						//!< Connect dialog
	
	LogPage logpage;										//!< Shows log messages (see LogPage and Log)
	ControlPage controlpage;						//!< Shows base controls (see FoamCtrl)
	
	typedef std::map<const string, DevicePage*> pagelist_t;
	pagelist_t pagelist;								//!< List all device pages here
	
	void on_about_activate();						//!< MainMenu::about button callback
	void on_quit_activate();						//!< MainMenu::quit button callback
	void on_connect_activate();					//!< MainMenu::connect button callback
	
	void disable_gui();									//!< Disable GUI elements
	void enable_gui();									//!< Enable GUI elements
	
public:	
	MainMenu menubar;
	
	void on_ctrl_connect_update();			//!< Connects to FoamControl::signal_connect()
	void on_ctrl_message_update();			//!< Connects to FoamControl::signal_message()
	void on_ctrl_device_update();				//!< Connects to FoamControl::signal_device()
	
	MainWindow();
	~MainWindow() {};
};

#endif // HAVE_FGUI_H

// GUI DOCUMENTATION //
/*********************/

/*!	
 \page fgui FOAM GUI
 
 \section aboutgui About the FOAM GUI
 
 Although FOAM can be controlled over the network by hand, it is easier
 and in some cases necessary to use a GUI. To this end, the FOAM GUI is
 supplied together with the framework. This document explains the structure
 used behind the GUI.
 
 \section guistruct FOAM GUI breakdown
 
 The GUI consists of two parts
 - The main controls (connect, start, stop, calibrate)
 - A tab for every device connected to FOAM
 
 The main controls are more or less the same every time, but the device tabs
 are opened depending on the hardware connected to the specific FOAM
 implementation you are trying to control. To make the implementation of new
 hardware easy on the GUI side as well, this has been set up similar to the
 way devices can be implemented in FOAM.
 
 At the root of the GUI, there is the Gtk::Window MainWindow. This class uses
 the FoamControl class to connect to FOAM, which is displayed as a tab in a 
 Gtk::Notebook by a ControlPage class. The FoamControl class inquires what
 kind of devices are connected to FOAM, and if these are supported by the GUI,
 they will be added to the interface as a DevicePage at run-time. The 
 DevicePage then issues a DeviceCtrl class to connect to the specific hardware 
 part (again, the UI and network IO are seperated). All these DevicePages are 
 stored in a list devlist which is part of MainWindow. Hierarchically this 
 goes more or less as follows:
 
 <ul>
 <li> MainWindow (fgui.cc) </li>
 <ul>
 <li> LogPage (logview.cc) </li>
 <ul><li> Log (log.cc)</li></ul>
 <li> ControlPage GUI (controlview.cc) </li>
 <ul><li> FoamControl I/O (foamcontrol.cc) </li></ul>
 <li> DevicePage (deviceview.cc) </li>
 <ul><li> DeviceCtrl (devicectrl.cc) </li></ul>
 </ul>
 </ul>
 
 To accomodate different types of hardware, specific classes are derived from
 both DevicePage and DeviceCtrl to tailor the UI and I/O to the specific 
 hardware. Sometimes these can be doubly derived, such as WfsPage and WfsCtrl.
  
 \section signals Connection, message and device signals
 
 To provide a connection between the UI and network IO, Glib::Dispatcher 
 instances are used. Every DeviceCtrl has one signal_update where DevicePage
 can connect callback functions to to respond to updates from FOAM. 
 Furthermore, FoamControl has three Glib::Dispatchers to notify ControlPage & 
 MainWindow of changes that occured.
 
 With regards to GUI changes, The general philosophy is: keep as much intact as possible, but reflect the 
 state of the system accurately. I.e. when not connected: disable the GUI, but 
 leave the last options intact (i.e. last active state of the GUI). When 
 connected (or more possibilities are available), enable or update gui 
 elements. 
 
 In practice this results in the following signal hierarchy:
 
 \subsection foamio Main FOAM I/O
 
 <ul>
 <li>FoamControl handles I/O for to and from FOAM</li>
 <ul>
 <li>on connect: request (device) info, call FoamControl::signal_connect()</li>
 <li>on disconnect: call FoamControl::signal_connect()</li>
 <li>on message: call FoamControl::signal_message()</li>
 <li>on devices: call FoamControl::signal_device()</li>
 </ul>
 <li>ControlPage handles the FoamControl GUI</li>
 <ul>
 <li>ControlPage::on_connect_update() connects to FoamControl::signal_connect() and enables the GUI</li>
 <li>ControlPage::on_connect_update() connects to FoamControl::signal_connect() and disables the GUI</li>
 <li>ControlPage::on_message_update() connects to FoamControl::signal_message() and update the GUI</li>
 </ul>
 <li>MainWindow only handles devices</li>
 <ul>
 <li>MainWindow::on_ctrl_device_update() connects to FoamControl::signal_device(), update device tabs as necessary</li>
 <li>MainWindow::on_ctrl_message_update() connects to FoamControl::signal_message(), placeholder (does nothing)</li>
 <li>MainWindow::on_ctrl_connect_update() connects to FoamControl::signal_connect(), placeholder (does nothing)</li>
 </ul>
 </ul>
 
 \section devctrl Devices
 
 The main aim of the GUI is to control devices running under FOAM. When 
 FoamControl connects to an instance of FOAM, it queries which devices are 
 connected to the system (see FoamControl::on_connected). This is processed
 by FoamControl::on_message and when new devices are found 
 FoamCtrl::signal_device() is triggered.
 
 When a new device is detected, the appropriate GUI class is instantiated and 
 added to the GUI. The GUI class will start a control connection to the device
 and handle I/O. The basic classes to achieve this are DevicePage and 
 DeviceCtrl. These can be overloaded to provide more detailed control over a
 device.
 
 \section moreinfo See also
 
 More information can be found on these pages:
 - \subpage dev_ui "Devices UI"
 

 */
 
