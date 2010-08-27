/*
 camera.h -- generic camera input/output wrapper
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.	If not, see <http://www.gnu.org/licenses/>. 
 */
/*! 
 @file camera.h
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl) and Guus Sliepen (guus@sliepen.org)
 @brief Generic camera class.
 
 This class extends the Device class and provides a base class for cameras. Does not implement anything itself.
 */

#ifndef HAVE_CAM_H
#define HAVE_CAM_H

#include <fstream>
#include <stdint.h>

#include "types.h"
#include "config.h"
#include "io.h"
#include "devices.h"

using namespace std;

static const string cam_type = "cam";

/*!
 @brief Base camera class. This should be overloaded with the specific camera class.

 
 The Camera class is a template for implementing camera software. The class 
 consists of two parts. One part of the class handles network I/O from outside
 (i.e. from a GUI), this is done with 'netio' in a seperate thread. The other
 part is hardware I/O which is done by a seperate thread through 'handler' in
 the 'camthr' thread. Graphically:
 
 Device --- netio --        --- netio ---
      \---- main --- Camera --- cam_thr -
												  \---- main ----
 
 \li netio gets input from outside (GUIs), reads from shared class
 \li cam_thr runs standalone, gets input from variables (configuration), 
		provides hooks through sigc++ slots.
 \li main thread calls camera functions to read out data/settings, can hook up 
		to slots to get 'instantaneous' feedback from cam_thr.
 
 Camera net IO
 
 Valid commends include
 
 \li quit, bye: disconnect from camera
 \li restart: restart camera
 \li set <prop>: set a property (see list below)
 \li get <prop>: get a property (see list below)
 \li thumnail: get a 32x32x8 thumbnail
 \li grab <x1> <y1> <x2> <y2> <scale> [darkflat] [histogram]: grab an image cropped from (x1,y1) to (x2,y2) and scaled down by factor scale. Darkflat and histogram are optional.
 \li dark [n]: grab <n> darkframes, otherwise take the default <ndark>
 \li flat [n]: grab <n> flatframes, otherwise take the default <nflat>
 \li statistics [n]: get statistics over the next n frames.
 
 
 Valid set properties:
 \li exposure
 \li interval
 \li gain
 \li offset
 \li filename
 \li outputdir
 \li fits
 
 Valid get properties:
 \li All set properties, plus:
 \li width
 \li depth
 \li height
 \li state
 \li 
 
 */ 
class Camera : public Device {
public:
	typedef enum {
		OFF = 0,
		WAITING,
		SINGLE,
		RUNNING,
		CONFIG,
		ERROR
	} mode_t;
	
	//!< Data structure for storing frames, taken from filter_control by Guus Sliepen
	typedef struct frame {
		void *data;						//!< Generic data pointer, might be necessary for some hardware
		void *image;					//!< Pointer to frame data
		uint32_t *histo;
		size_t id;
		struct timeval tv;
		
		frame() {
			data = 0;
			image = 0;
			id = 0;
			histo = 0;
		}
		
		double avg;
		double rms;
		
		double cx;						//!< @todo ???
		double cy;						//!< @todo ???
		double cr;						//!< @todo ???
		
		double rms1;
		double rms2;
		
		double dx;
		double dy;
	} frame_t;
	
protected:
	pthread::thread cam_thr;			//!< Camera hardware thread.
	pthread::mutex cam_mutex;
	pthread::cond cam_cond;
	
	pthread::cond mode_cond;			//!< Mode change notification
	pthread::mutex mode_mutex;
	
	// These should be implemented in derived classes:
	virtual void cam_handler() = 0;										//!< Camera handler
	virtual void cam_set_exposure(double value) = 0;	//!< Set exposure in camera hardware
	virtual void cam_set_interval(double value) = 0;	//!< Set interval in camera hardware
	virtual void cam_set_gain(double value) = 0;			//!< Set gain in camera hardware
	virtual void cam_set_offset(double value) = 0;		//!< Set offset in camera hardware
	virtual void cam_set_mode(mode_t newmode) = 0;		//!< Set mode for cam_handler()

	virtual void do_restart() = 0;

	void *cam_queue(void *data, void *image, struct timeval *tv = 0); //!< Store frame in buffer, returns oldest frame if buffer is full
	
	void calculate_stats(frame *frame);
	static frame_t *get_frame(size_t id, bool wait = true);

	frame_t *frames;							//!< Frame ringbuffer
	uint64_t count;
	size_t nframes;
	uint64_t timeouts;
	
	//! @todo incorporate dark/flat into struct or class?
	size_t ndark;									//!< Number of frames used in darkframe
	size_t nflat;									//!< Number of frames used in flatframe
	frame_t dark;
	frame_t flat;
	double darkexp;								//!< Exposure used for darkexp
	double flatexp;								//!< Exposure used for flatexp

	double interval;							//!< Frame time (exposure + readout)
	double exposure;							//!< Exposure time
	double gain;									//!< Camera gain
	double offset;								//!< @todo What is this?
	
	coord_t res;									//!< Camera pixel resolution
	int depth;										//!< Camera pixel depth in bits
	dtype_t dtype;								//!< Camera datatype

	mode_t mode;									//!< Camera mode (see mode_t)
	
	static string filenamebase;					//!< Base filename, input for makename()
	string outputdir;							//!< Output dir for saving files, absolute or relative to ptc->datadir
	
	string fits_observer;					//!< FITS header properties for saved files
	string fits_target;						//!< FITS header properties for saved files
	string fits_comment;					//!< FITS header properties for saved files
	
public:
	Camera(Io &io, foamctrl *ptc, string name, string type, string port, string conffile);
	virtual ~Camera();

	double get_interval() { return interval; }
	double get_exposure() { return exposure; }
	double get_gain() { return gain; }
	int get_width() { return res.x; }
	int get_height() { return res.y; }
	coord_t get_res() { return res; }
	int get_depth() { return depth; }
	uint16_t get_maxval() { return (1 << depth); }
	dtype_t get_dtype() { return dtype; }

	mode_t get_mode() { return mode; }
	
	frame_t *get_frame(bool block) {	
		if (block) {
			cam_mutex.lock();
			cam_cond.wait(cam_mutex);
			cam_mutex.unlock();
		}
		return &frames[count % nframes];
	}
	
	
	// From Devices::
	virtual int verify() { return 0; }
	virtual void on_message(Connection*, std::string);
	
	void set_exposure(Connection *conn, double value);
	void set_interval(Connection *conn, double value);
	void set_gain(Connection *conn, double value);
	void set_offset(Connection *conn, double value);
	void set_mode(Connection *conn, string value);
	void get_fits(Connection *conn);
	void set_fits(Connection *conn, string line);
	void set_filename(Connection *conn, string value);
	void set_outputdir(Connection *conn, string value);
	
	std::string makename(const string &base = filenamebase);
	
	void thumbnail(Connection *conn);
	static void grab(Connection *conn, int x1, int y1, int x2, int y2, int scale, bool do_df, bool do_histo);
	
	static void accumfix();
	static void darkburst(Connection *conn, size_t bcount);
	static void flatburst(Connection *conn, size_t bcount);
	static bool accumburst(uint32_t *accum, size_t bcount);
	
	static void statistics(Connection *conn, size_t bcount);	
};


#endif /* HAVE_CAM_H */
