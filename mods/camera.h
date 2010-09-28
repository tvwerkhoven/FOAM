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

const string cam_type = "cam";

/*!
 @brief Base camera class. This should be overloaded with the specific camera class.

 
 The Camera class is a template for implementing camera software. The class 
 consists of two parts. One part of the class handles network I/O from outside
 (i.e. from a GUI), this is done with 'netio' in a seperate thread. The other
 part is hardware I/O which is done by a seperate thread through 'handler' in
 the 'camthr' thread. Graphically:
 
 Device --- netio --        --- netio ----
      \---- main --- Camera --- cam_thr --
												  +---- proc_thr -
												  \----	main -----
 
 \li netio gets input from outside (GUIs), reads from shared class
 \li cam_thr runs standalone, gets input from variables (configuration), 
		provides hooks through sigc++ slots.
 \li proc_thr runs some processing over the captured frames from cam_thr if 
		necessary, responds to cam_cond() broadcasts
 \li main thread calls camera functions to read out data/settings, can hook up 
		to slots to get 'instantaneous' feedback from cam_thr.
 
 Capture process
 
 \li cam_thr captures frame (needs to be implemented in cam_handler()), calls cam_queue()
 \li cam_queue() locks cam_mut
 
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
 \li mode
 
 Valid get properties:
 \li All set properties, plus:
 \li width
 \li depth
 \li height

 @todo What to do with mode & state?
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
	
	string mode2str(const mode_t &m) {
		if (m == OFF) return "OFF";
		if (m == WAITING) return "WAITING";
		if (m == SINGLE) return "SINGLE";
		if (m == RUNNING) return "RUNNING";
		if (m == CONFIG) return "CONFIG";
		if (m == ERROR) return "ERROR";
		return "";
	}
	mode_t str2mode(const string &m) {
		if (m == "OFF") return OFF;
		if (m == "WAITING") return WAITING;
		if (m == "SINGLE") return SINGLE;
		if (m == "RUNNING") return RUNNING;
		if (m == "CONFIG") return CONFIG;
		if (m == "ERROR") return ERROR;
		return OFF;
	}
	
	//!< Data structure for storing frames, taken from filter_control by Guus Sliepen
	typedef struct frame {
		void *data;						//!< Generic data pointer, might be necessary for some hardware
		void *image;					//!< Pointer to frame data
		uint32_t *histo;
		size_t id;
		struct timeval tv;
		
		bool proc;						//!< Was the frame processed in time?
		
		frame() {
			data = 0;
			image = 0;
			id = 0;
			histo = 0;
			proc = false;
		}
		
		double avg;
		double rms;
	} frame_t;
	
protected:
	pthread::thread cam_thr;			//!< Camera hardware thread.
	pthread::mutex cam_mutex;			//!< Mutex used to limit access to frame data
	pthread::cond cam_cond;				//!< Cond used to signal threads about new frames
	
	pthread::thread proc_thr;			//!< Processing thread (only camera stuff)
	pthread::mutex proc_mutex;		//!< Cond/mutexpair used by cam_thr to notify proc_thr
	pthread::cond	proc_cond;			//!< Cond/mutexpair used by cam_thr to notify proc_thr
	
	pthread::cond mode_cond;			//!< Mode change notification
	pthread::mutex mode_mutex;
	
	// These should be implemented in derived classes:
	virtual void cam_handler() = 0;										//!< Camera handler
	virtual void cam_set_exposure(double value) = 0;	//!< Set exposure in camera
	virtual double cam_get_exposure() = 0;						//!< Get exposure from camera
	virtual void cam_set_interval(double value) = 0;	//!< Set interval in camera
	virtual double cam_get_interval() = 0;						//!< Get interval from camera
	virtual void cam_set_gain(double value) = 0;			//!< Set gain in camera
	virtual double cam_get_gain() = 0;								//!< Get gain from camera
	virtual void cam_set_offset(double value) = 0;		//!< Set offset in camera
	virtual double cam_get_offset() = 0;							//!< Get offset from camera

	virtual void cam_set_mode(mode_t newmode) = 0;		//!< Set mode for cam_handler()
	virtual void do_restart() = 0;

	void *cam_queue(void *data, void *image, struct timeval *tv = 0); //!< Store frame in buffer, returns oldest frame if buffer is full
	void cam_proc();																	//!< Process frames (if necessary)

	void calculate_stats(frame *frame);								//!< Calculate rms and such
	bool accumburst(uint32_t *accum, size_t bcount);	//!< For dark/flat acquisition
	void statistics(Connection *conn, size_t bcount);	//!< Post back statistics
	
	std::string makename(const string &base);					//!< Make filename from outputdir and filenamebase
	std::string makename() { return makename(filenamebase); }
	bool store_frame(frame_t *frame);									//!< Store frame to disk
	
	uint8_t *get_thumbnail(Connection *conn);					//!< Get 32x32x8 thumnail
	void grab(Connection *conn, int x1, int y1, int x2, int y2, int scale, bool do_df, bool do_histo);
	void accumfix();

	const uint8_t df_correct(const uint8_t *in, size_t offset);
	const uint16_t df_correct(const uint16_t *in, size_t offset);
	
	frame_t *frames;							//!< Frame ringbuffer
	size_t nframes;								//!< Ringbuffer size
	uint64_t count;								//!< Total number of frames captured
	uint64_t timeouts;						//!< Number of timeouts that occurred
	
	//! @todo incorporate dark/flat into struct or class?
	size_t ndark;									//!< Number of frames used in darkframe
	size_t nflat;									//!< Number of frames used in flatframe
	frame_t dark;									//!< Dark frame, dark.image is a sum of ndark frames, type is uint32_t.
	frame_t flat;									//!< Flat frame, flat.image is a sum of nflat frames, type is uint32_t.
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
	
	string filenamebase;					//!< Base filename, input for makename()
	string outputdir;							//!< Output dir for saving files, absolute or relative to ptc->datadir
	size_t nstore;								//!< Numebr of new frames to store (-1 for unlimited)

	void fits_init_phdu(char *phdu);	//!< Init FITS header unit
	bool fits_add_card(char *phdu, const string &key, const string &value); //!< Add FITS header card
	bool fits_add_comment(char *phdu, const string &comment); //!< Add FITS comment
	
	string fits_telescope;				//!< FITS header properties for saved files
	string fits_observer;					//!< FITS header properties for saved files
	string fits_instrument;				//!< FITS header properties for saved files
	string fits_target;						//!< FITS header properties for saved files
	string fits_comments;					//!< FITS header properties for saved files
	
public:
	Camera(Io &io, foamctrl *ptc, string name, string type, string port, string conffile);
	virtual ~Camera();

	double get_exposure() { return exposure; }
	double get_interval() { return interval; }
	double get_gain() { return gain; }
	double get_offset() { return offset; }
	mode_t get_mode() { return mode; }

	int get_width() { return res.x; }
	int get_height() { return res.y; }
	coord_t get_res() { return res; }
	int get_depth() { return depth; }
	uint16_t get_maxval() { return (1 << depth); }
	dtype_t get_dtype() { return dtype; }
	
	frame_t *get_frame(size_t id, bool wait = true);
	frame_t *get_last_frame();
	
	// From Devices::
	virtual int verify() { return 0; }
	virtual void on_message(Connection*, std::string);
	
	double set_exposure(double value);
	int set_store(int value);
	double set_interval(double value);
	double set_gain(double value);
	double set_offset(double value);
	mode_t set_mode(mode_t mode);
protected:
	void get_fits(Connection *conn);
	void set_fits(string line);
public:
	string set_fits_observer(string val);
	string set_fits_target(string val);
	string set_fits_comments(string val);
	string set_filename(string value);
	string set_outputdir(string value);
	
	void store_frames(int n=-1) { nstore = n; }
	
	int darkburst(size_t bcount);
	int flatburst(size_t bcount);
};


#endif /* HAVE_CAM_H */
