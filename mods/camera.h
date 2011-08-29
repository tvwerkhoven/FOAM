/*
 camera.h -- generic camera input/output wrapper
 Copyright (C) 2009--2011 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
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

#ifndef HAVE_CAM_H
#define HAVE_CAM_H

#include <fstream>
#include <stdint.h>
#include <limits.h>
#include <fitsio.h>

#include "types.h"
#include "config.h"
#include "io.h"
#include "path++.h"

#include "devices.h"

using namespace std;

const string cam_type = "cam";

/*!
 @brief Base camera class. This should be overloaded with the specific camera class.

 The Camera class is a template for implementing camera software. The class 
 consists of two parts. One part of the class handles network I/O from outside
 (i.e. from a GUI), this is done with 'netio' in a seperate thread and is 
 initiated from the Device class. The other part is hardware I/O which is done
 by a seperate thread through the 'cam_handler' method in the 'camthr' thread. A
 third thread 'proc_thr' handles processing of the image data in 'cam_proc()'.
 
 Graphically:
 
 <tt>
 Device --- netio --        --- netio (on_message/on_connect)
      \---- main --- Camera --- cam_thr (cam_handler) -----
												  +---- proc_thr (cam_proc) -------
													\----	main (returns after init) -
 </tt>
 
 \li netio gets input from outside (GUIs), reads from shared class
 \li cam_thr runs standalone, gets input from variables (configuration), 
		provides hooks through sigc++ slots.
 \li proc_thr runs some processing over the captured frames from cam_thr if 
		necessary, responds to cam_cond() broadcasts
 \li main thread calls camera functions to read out data/settings, can hook up 
		to slots to get 'instantaneous' feedback from cam_thr.
 
 \section cam_cap Capture process
 
 \li cam_thr captures frame (needs to be implemented in derived classes in cam_handler()), calls cam_queue()
 \li cam_queue() locks cam_mut
 
 \section cam_netio Network IO
 
 Valid commends include:
 
 \li quit, bye: disconnect from camera
 \li restart: restart camera
 \li set <prop>: set a property (see list below)
 \li get <prop>: get a property (see list below)
 \li thumnail: get a 32x32x8 thumbnail
 \li grab <x1> <y1> <x2> <y2> <scale> [darkflat] [histogram]: grab an image cropped from (x1,y1) to (x2,y2) and scaled down by factor scale. Darkflat and histogram are optional.
 \li dark [n]: grab <n> darkframes, otherwise take the default <ndark>
 \li flat [n]: grab <n> flatframes, otherwise take the default <nflat>
 
 Valid set properties:
 \li exposure
 \li interval
 \li gain
 \li offset
 \li filename
 \li fits
 \li mode
 
 Valid get properties:
 \li All set properties, plus:
 \li width
 \li depth
 \li height
 
 \section cam_cfg Configuration parameters
 
 The Camera class supports the following configuration parameters, with 
 defaults between brackets:
 - nframes (8): Camera::nframes
 - ndark (10): Camera::ndark
 - nflat (10): Camera::nflat
 - interval (1.0): Camera::interval
 - exposure (1.0): Camera::exposure
 - gain (1.0): Camera::gain
 - offset (0.0): Camera::offset
 - width (512): Camera::res
 - height (512): Camera::res
 - depth (8): Camera::depth
 
 \section cam_calib Calibration
 
 A camera is calibrated when dark & flat frames are available with the current
 exposure settings (exposure, gain, offset).
 
 \section cam_todo Todo
 
 - Add 'camera calibrated' check. Need flat & dark, cannot store in one bool. -> is_calibrated() ?
 - Store dark/flat bursts (as FITS)
 - Save dark/flat filenames in configuration
 - Re-load dark/flat bursts at start
 - Implement guard-pixel watchers (for fast processing, i.e. SHWFS)
 
 */ 
class Camera: public foam::Device {
	// Wfs is a friend class because it needs more access to the camera (also mutexes etc)
	friend class Wfs;
public:
	typedef enum {
		OFF = 0,
		WAITING,
		SINGLE,
		RUNNING,
		CONFIG,
		ERROR
	} mode_t;
	
	string mode2str(const mode_t &m) const {
		if (m == OFF) return "OFF";
		if (m == WAITING) return "WAITING";
		if (m == SINGLE) return "SINGLE";
		if (m == RUNNING) return "RUNNING";
		if (m == CONFIG) return "CONFIG";
		if (m == ERROR) return "ERROR";
		return "";
	}
	mode_t str2mode(const string &m) const {
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
		void *image;					//!< Pointer to frame data (unsigned int, 8 or 16 bpp)
		uint32_t *histo;			//!< Histogram data (optional)
		size_t id;						//!< Unique frame ID
		size_t size;					//!< Size of 'image' [bytes]
		coord_t res;					//!< Resolution of 'image' [pixels]
		int depth;						//!< Data depth for 'image' [bits]
		size_t npixels;				//!< Number of pixels in frame
		struct timeval tv;		//!< Frame creation timestamp (as close as possible)
		
		bool proc;						//!< Was the frame processed?
		
		frame() {
			data = 0;
			image = 0;
			histo = 0;
			id = 0;
			size = 0;
			proc = false;
			avg = 0;
			rms = 0;
			min = INT_MAX;
			max = 0;
		}
		
		double avg;
		double rms;
		int min;
		int max;
	} frame_t;
	
protected:
	pthread::thread cam_thr;			//!< Camera hardware thread.
	pthread::mutex cam_mutex;			//!< Mutex used to limit access to frame data
	pthread::cond cam_cond;				//!< Cond used to signal threads about new frames
	
	pthread::thread proc_thr;			//!< Processing thread (only camera stuff)
	pthread::mutex proc_mutex;		//!< Cond/mutexpair used by cam_thr to notify proc_thr
	pthread::cond	proc_cond;			//!< Cond/mutexpair used by cam_thr to notify proc_thr
	
	pthread::cond mode_cond;			//!< Camera::mode change notification
	pthread::mutex mode_mutex;		//!< Camera::mode change notification
	
	// These should be implemented in derived classes:
	virtual void cam_handler() = 0;											//!< Camera handler
	virtual void cam_set_exposure(const double value)=0;	//!< Set exposure in camera
	virtual double cam_get_exposure() =0;						//!< Get exposure from camera
	virtual void cam_set_interval(const double value)=0;	//!< Set interval in camera
	virtual double cam_get_interval() =0;						//!< Get interval from camera
	virtual void cam_set_gain(const double value)=0;			//!< Set gain in camera
	virtual double cam_get_gain() =0;								//!< Get gain from camera
	virtual void cam_set_offset(const double value)=0;		//!< Set offset in camera
	virtual double cam_get_offset() =0;							//!< Get offset from camera

	virtual void cam_set_mode(const mode_t newmode)=0; //!< Set mode for cam_handler()
	virtual void do_restart()=0;

	void *cam_queue(void *const data, void *const image, struct timeval *const tv = 0); //!< Store frame in buffer, returns oldest frame if buffer is full
	void cam_proc();																	//!< Process frames (if necessary)

	void calculate_stats(frame *const frame) const;		//!< Calculate rms and such
	bool accumburst(uint32_t *accum, size_t bcount);	//!< For dark/flat acquisition
//	void statistics(Connection *conn, size_t bcount);	//!< Post back statistics
	
	Path makename(const string &base) const;					//!< Make filename from outputdir and filenamebase
	Path makename() const { return makename(filenamebase); }
	int store_frame(const frame_t *const frame) const;	//!< Store frame to disk
	
	uint8_t *get_thumbnail(Connection *conn);					//!< Get 32x32x8 thumnail
	void grab(Connection *conn, int x1, int y1, int x2, int y2, int scale, bool do_df, bool do_histo);

	uint8_t df_correct(const uint8_t *in, size_t offset);
	uint16_t df_correct(const uint16_t *in, size_t offset);
	
	bool do_proc;									//!< Do frame-processing or not?
	
	frame_t *frames;							//!< Frame ringbuffer
	size_t nframes;								//!< Ringbuffer size
	size_t count;									//!< Total number of frames captured
	size_t timeouts;							//!< Number of timeouts that occurred
	
	//! @todo incorporate dark/flat into struct or class?
	size_t ndark;									//!< Number of frames used in Camera::darkframe
	size_t nflat;									//!< Number of frames used in Camera::flatframe
	frame_t dark;									//!< Dark frame, dark.image is a sum of Camera::ndark frames, type is uint32_t.
	frame_t flat;									//!< Flat frame, flat.image is a sum of Camera::nflat frames, type is uint32_t.
	double darkexp;								//!< Exposure used for darkimage
	double flatexp;								//!< Exposure used for flatimage

	double interval;							//!< Frame time (exposure + readout)
	double exposure;							//!< Exposure time
	double gain;									//!< Camera gain
	double offset;								//!< Constant offset added to frames
	
	coord_t res;									//!< Camera pixel resolution
	int depth;										//!< Camera pixel depth in bits @todo Is now ceil'ed to 8, 16 or 32. Need to fix real value here

	mode_t mode;									//!< Camera mode (see Camera::mode_t)
	
	string filenamebase;					//!< Base filename, input for makename()
	ssize_t nstore;								//!< Numebr of new frames to store (-1 for unlimited)

	int fits_add_card(fitsfile *fptr, string key, string value, string comment="") const; //!< Shorthand for writing a header card.

	string fits_telescope;				//!< FITS header properties for saved files
	string fits_observer;					//!< FITS header properties for saved files
	string fits_instrument;				//!< FITS header properties for saved files
	string fits_target;						//!< FITS header properties for saved files
	string fits_comments;					//!< FITS header properties for saved files
	
	int conv_depth(const int d) { 
		if (d<=8) return 8;
		if (d<=16) return 16;
		if (d<=32) return 32;
		if (d<=64) return 64;
		return -1;
	}
		
	
public:
	Camera(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online=true);
	virtual ~Camera();

	double get_exposure() const { return exposure; }
	double get_interval() const { return interval; }
	double get_gain() const { return gain; }
	double get_offset() const { return offset; }
	mode_t get_mode() const { return mode; }

	int get_width() const { return res.x; }
	int get_height() const { return res.y; }
	coord_t get_res() const { return res; }
	int get_depth() const { return depth; }
	size_t get_maxval() const { return (1 << depth); }
	
	frame_t *get_next_frame(const bool wait=true);
	frame_t *get_last_frame() const;
protected:
	//! @todo Not allowed to call this from outside, cam_mutex needs to be locked outside this function
	frame_t *get_frame(const size_t id, const bool wait=true);
public:
	size_t get_count() const { return count; }
	size_t get_bufsize() const { return nframes; }
	
	void set_proc_frames(const bool b=true) { do_proc = b; }
	bool get_proc_frames() const { return do_proc; }
	
	// From Devices::
	virtual int verify() { return 0; }
	virtual void on_message(Connection*, string);
	
	double set_exposure(const double value);
	int set_store(const int value);
	double set_interval(const double value);
	double set_gain(const double value);
	double set_offset(const double value);
	mode_t set_mode(const mode_t mode);
protected:
	void get_fits(const Connection *const conn) const ;
	void set_fits(string line);
public:
	string set_fits_observer(const string val);
	string set_fits_target(const string val);
	string set_fits_comments(const string val);
	string set_filename(const string value);
	
	void store_frames(const int n=-1) { nstore = n; }
	
	int darkburst(size_t bcount);
	int flatburst(size_t bcount);
};


#endif /* HAVE_CAM_H */

/*!
 \page dev_cam Camera devices
 
 The Camera class provides control for cameras.
 
 \section dev_cam_der Derived classes
 - \subpage dev_cam_dummy "Dummy camera device"
 - \subpage dev_cam_fw1394 "FW1394 camera device"
 - \subpage dev_cam_imgcam "Image camera device"
 - \subpage dev_cam_simulcam "Simulation camera device"
 - \subpage dev_cam_andor "Andor iXon camera device"

 */
