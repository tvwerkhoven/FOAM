/*
 camctrl.cc -- camera control class
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 Copyright (C) 2010 Guus Sliepen
 
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
 @file camctrl.h
 @brief Camera UI control 
 */

#ifndef HAVE_CAMCTRL_H
#define HAVE_CAMCTRL_H

#include <glibmm/dispatcher.h>
#include <string>

#include "pthread++.h"
#include "protocol.h"

#include "devicectrl.h"

/*!
 @brief Generic camera control class
 
 This class controls generic cameras. On top of the control connection
 DeviceCtrl::protocol, it has another CamCtrl::monitorprotocol for bulk-data
 transport (images) such that control does not get congested. Two extra 
 signals, signal_thumbnail and signal_monitor are added which fire when a new
 thumbnail is available (whic does go over the control connection), or a new
 regular image (which goes over monitorprotocol).
 
 Images from the camera are stored in CamCtrl::monitor, where some metadata
 is also included. Write-access to the data is managed by 
 CamCtrl::monitor.mutex.
 */
class CamCtrl: public DeviceCtrl {
public:
	typedef enum {
		OFF = 0,
		WAITING,
		SINGLE,
		RUNNING,
		CONFIG,
		ERROR,
		UNDEFINED
	} mode_t;														//!< Camera runmode
	mode_t mode;
	
protected:
	Protocol::Client monitorprotocol;		//!< Data channel for images

	// Camera settings
	double exposure;										//!< Camera exposure
	double interval;										//!< Camera time between frames (inverse framerate)
	double gain;												//!< Camera gain
	double offset;											//!< Camera offset
	int32_t width;											//!< Camera horizontal number of pixels
	int32_t height;											//!< Camera vertical number of pixels
	int32_t depth;											//!< Camera bitdepth
	std::string filename;								//!< Filename camera will store data to
	int32_t nstore;											//!< How many upcoming frames will be stored

	// From DeviceCtrl::
	virtual void on_message(std::string line);
	virtual void on_connected(bool connected);

	// For monitorprotocol
	void on_monitor_message(std::string line);
	void on_monitor_connected(bool connected);
	
public:
	CamCtrl(Log &log, const std::string name, const std::string host, const std::string port);
	~CamCtrl();

	uint8_t thumbnail[32 * 32];
	// Camera frame
	struct monitor {
		monitor() {
			image = 0;
			size = 0;
			x1 = 0;
			x2 = 0;
			y1 = 0;
			y2 = 0;
			scale = 1;
			depth = 0;
			avg=0;
			rms=0;
			min=INT_MAX;
			max=0;
			histo = 0;
		}
		pthread::mutex mutex;							//!< Write-access mutex to image
		void *image;											//!< Pointer to data
		size_t size;											//!< Bytesize of image
		int x1;														//!< Position of this frame wrt the original frame (x1, y2) to (x2, y2)
		int y1;														//!< Position of this frame wrt the original frame (x1, y2) to (x2, y2)
		int x2;														//!< Position of this frame wrt the original frame (x1, y2) to (x2, y2)
		int y2;														//!< Position of this frame wrt the original frame (x1, y2) to (x2, y2)
		int scale;												//!< Spatial scaling, 1=every pixel, 2=every second pixel, etc.

		double avg;
		double rms;
		int min;
		int max;
		uint32_t *histo;									//!< Histogram (optional)
		int depth;												//!< Depth of this frame
	} monitor;													//!< Stores frames from the camera. Note that these frames can be cropped and/or scaled wrt the original frame.
	
	// Get & set settings
	double get_exposure() const;				//!< Get camera exposure
	double get_interval() const;				//!< Get time between frames (inverse framerate)
	double get_gain() const;						//!< Get gain
	double get_offset() const;					//!< Get offset
	int32_t get_width() const;					//!< Get horizontal number of pixels
	int32_t get_height() const;					//!< Get vertical number of pixels
	int32_t get_depth() const;					//!< Get camera bitdepth
	std::string get_filename() const;		//!< Get filename
	void get_thumbnail();								//!< Send request for a thumnail
	mode_t get_mode() const { return mode; } //!< Get current camera mode
	std::string get_modestr(const mode_t m) const; //!< Get a mode as a string
	std::string get_modestr() { return get_modestr(mode); } //!< Get current camera mode as string
	int32_t get_nstore() { return nstore; } //!< Get number of frames that will be stored
	
	void set_exposure(double value);		//!< Set camera exposure
	void set_interval(double value);		//!< Set time between frames (inverse framerate)
	void set_gain(double value);				//!< Set gain
	void set_offset(double value);			//!< Set offset
	void set_filename(const std::string &filename); //!< Set filename
	void set_fits(const std::string &fits); //!< Set FITS paramets that will be stored in the header
	void set_mode(const mode_t m);			//!< Change camera mode

	// Take images
	void darkburst(int count);					//!< Take a burst of darkfield images
	void flatburst(int count);					//!< Take a burst of flatfield images
	void burst(int count, int fsel = 0); //!< Take a burst of images
	void grab(int x1, int y1, int x2, int y2, int scale = 1, bool df_correct = false); //!< Grab an image from the camera, crop it from (x1, y1) to (x2, y2) and take every 'scale'th pixel. Optically dark & flatfield correct it.
	void store(int nstore);							//!< Store upcoming nstore frames to disk
	
	Glib::Dispatcher signal_thumbnail;	//!< New thumbnail available
	Glib::Dispatcher signal_monitor;		//!< New frame (crop) available
};

#endif // HAVE_CAMCTRL_H
