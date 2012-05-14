/*
 cam.cc -- generic camera input/output wrapper
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

#include <string>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <sys/stat.h>
#ifdef LINUX
#define _XOPEN_SOURCE 600	// for posix_fadvise()
#endif
#include <fcntl.h>
#include <fitsio.h>

#include "io.h"
#include "utils.h"
#include "config.h"
#include "path++.h"

#include "devices.h"
#include "camera.h"

using namespace std;

Camera::Camera(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online):
Device(io, ptc, name, cam_type + "." + type, port, conffile, online),
do_proc(true), nframes(-1), count(0), timeouts(0), ndark(10), nflat(10), 
darkexp(1.0), flatexp(1.0),
interval(1.0), exposure(1.0), gain(1.0), offset(0.0), 
res(0,0), depth(-1),
mode(Camera::OFF),
filenamebase("FOAM"), nstore(0),
fits_telescope("undef"), fits_observer("undef"), fits_instrument("undef"), fits_target("undef"), fits_comments("undef")
{
	io.msg(IO_DEB2, "Camera::Camera()");
	// Register network commands with base device:
	add_cmd("quit");
	add_cmd("restart");
	add_cmd("set mode");
	add_cmd("set exposure");
	add_cmd("set interval");
	add_cmd("set gain");
	add_cmd("set offset");
	add_cmd("set filename");
	add_cmd("set fits");
	add_cmd("get mode");
	add_cmd("get exposure");
	add_cmd("get interval");
	add_cmd("get gain");
	add_cmd("get offset");
	add_cmd("get width");
	add_cmd("get height");
	add_cmd("get depth");
	add_cmd("get resolution");
	add_cmd("get filename");
	add_cmd("get fits");
	add_cmd("thumbnail");
	add_cmd("grab");
	add_cmd("store");
	add_cmd("dark");
	add_cmd("flat");
//	add_cmd("statistics");
	
	// Set buffer size (default 32 frames)
	nframes = cfg.getint("nframes", 32);
	frames = new frame_t[nframes];
	
	// Set number of darks & flats to take (default 10)
	ndark = cfg.getint("ndark", 10);
	nflat = cfg.getint("nflat", 10);
	
	// Set interval, exposure, gain and offset
	interval = cfg.getdouble("interval", 1.0);
	exposure = cfg.getdouble("exposure", 1.0);
	gain = cfg.getdouble("gain", 1.0);
	offset = cfg.getdouble("offset", 0.0);
	
	// Set frame resolution & bitdepth
	res.y = cfg.getint("height", 512);
	res.x = cfg.getint("width", 768);
	depth = cfg.getint("depth", 8);

//	io.msg(IO_XNFO, "Camera::Camera(): %dx%dx%d, exp:%g, int:%g, gain:%g, off:%g",
//				 res.x, res.y, depth, exposure, interval, gain, offset);
	
	// Set output dir because this class might store files.
	set_outputdir("");

	proc_thr.create(sigc::mem_fun(*this, &Camera::cam_proc));
}

Camera::~Camera() {
	io.msg(IO_DEB2, "Camera::~Camera()");
	proc_thr.cancel();
	proc_thr.join();
	
	// Delete the camera ringbuffer here. The only memory we free here is the
	// array of *references* to frames and the histogram memory. The image data
	// itself (frames.data and frames.image) should be free'd by the derived 
	// classes because we don't know what kind of object it is here.
	for (size_t i=0; i<nframes; i++)
		delete[] frames[i].histo;
	delete[] frames;
}

void Camera::cam_proc() {
	io.msg(IO_DEB2, "Camera::cam_proc()");
	frame_t *frame;
	
	while (true) {
		// Always wait for proc_cond broadcasts
		{
			pthread::mutexholder h(&proc_mutex);
			proc_cond.wait(proc_mutex);
		}
		
		// Lock cam_mutex before handling the data
		pthread::mutexholder h(&cam_mutex);
		
		// There is a new frame ready now, process it
		frame = get_last_frame();
		if (do_proc)
			calculate_stats(frame);
		 
		if (nstore == -1 || nstore > 0) {
//			io.msg(IO_DEB2, "Camera::cam_proc() nstore=%d", nstore);
			int status=store_frame(frame);
			if (!status) {
				nstore--;
				net_broadcast(format("ok store %d", nstore), "store");
//				io.msg(IO_DEB2, "Camera::cam_proc() nstore--", nstore);
			} else {
				net_broadcast(format("error storing frame: %d", status), "store");
				io.msg(IO_ERR, "Camera::cam_proc() fits store error: %d", status);
			}
		}

		// Flag frame as processed
		frame->proc = true;

		// Notify all threads waiting for new frames now
		cam_cond.broadcast();
	}
}

int Camera::fits_add_card(fitsfile *fptr, string key, string value, string comment) const {
	int status=0;
	char hdr_val[FLEN_CARD], hdr_comm[FLEN_CARD];
	snprintf(hdr_val, FLEN_CARD, "%s", value.c_str());
	snprintf(hdr_comm, FLEN_CARD, "%s", comment.c_str());
	
//	fits_update_key(fptr, TSTRING, "MAXVAL", hdr_str, "Maximum data value", &status);
	fits_update_key(fptr, TSTRING, key.c_str(), hdr_val, hdr_comm, &status);
	return status;
}

int Camera::store_frame(const frame_t *const frame) const {
//	if (depth != 8 && depth != 16) {
//		io.msg(IO_WARN, "Camera::store_frame() Only 8 and 16 bit images supported!");
//		return false;
//	}
	
	// Generate path to store file to, based on filenamebase
	Path filename = makename();
	io.msg(IO_DEB1, "Camera::store_frame(%p) to %s", frame, filename.c_str());
	
	fitsfile *fptr;
  int status = 0, naxis = 2;
  long fpixel = 1, nelements = -1;
  long naxes[naxis];
	nelements = frame->npixels;
  naxes[0] = frame->res.x; naxes[1] = frame->res.y;

	// Open file
	fits_create_file(&fptr, filename.c_str(), &status);
	if (status) return status;
	
	// Get filedatatype parameters
	int ftype=0, dtype=0;
	if (frame->depth <= 8) {
		ftype = BYTE_IMG; dtype = TBYTE;
		io.msg(IO_DEB1, "Camera::store_frame() BYTE_IMG TBYTE");
	} else if (frame->depth <= 16) {
		ftype = USHORT_IMG; dtype = TUSHORT;
		io.msg(IO_DEB1, "Camera::store_frame() SHORT_IMG TUSHORT");
	} else if (frame->depth <= 32) {
		ftype = ULONG_IMG; dtype = TUINT;
		io.msg(IO_DEB1, "Camera::store_frame() LONG_IMG TUINT");
	} else {
		io.msg(IO_ERR, "Camera::store_frame() saving 32+ bit data not supported");
		return OVERFLOW_ERR;
	}
	
	// Create image 
	fits_create_img(fptr, ftype, naxis, naxes, &status);
	if (status) return status;
	
	// Write header: http://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/c_user/node39.html
	
	// Write a keyword; must pass the ADDRESS of the value
	fits_write_date(fptr, &status);

	if (frame->min < frame->max) {
		fits_add_card(fptr, "MAXVAL", format("%d", frame->max), "Max data value");
		fits_add_card(fptr, "MINVAL", format("%d", frame->min), "Min data value");
	}
	if (frame->avg != frame->rms) {
		fits_add_card(fptr, "AVG", format("%lf", frame->avg), "Average data value");
		fits_add_card(fptr, "RMS", format("%lf", frame->rms), "Data root mean square");
	}
	
	fits_add_card(fptr, "ORIGIN", PACKAGE_NAME " -- " PACKAGE_VERSION);
	fits_add_card(fptr, "DEVNAME", name, "FOAM device name");
	fits_add_card(fptr, "DEVTYPE", type, "FOAM device type");
	fits_add_card(fptr, "TELESCOPE", fits_telescope);
	fits_add_card(fptr, "INSTRUMENT", fits_instrument);
	fits_add_card(fptr, "OBSERVER", fits_observer);
	fits_add_card(fptr, "TARGET", fits_target);
	fits_add_card(fptr, "EXPTIME", format("%lf", exposure), "[s] Exposure time");
	fits_add_card(fptr, "INTERVAL", format("%lf", interval), "[s] Frame cadence");
	fits_add_card(fptr, "GAIN", format("%lf", gain));
	fits_add_card(fptr, "OFFSET", format("%lf", offset));
	if (status) return status;
	
	fits_write_comment(fptr, fits_comments.c_str(), &status);
	if (status) return status;

	// Write the array of integers to the image
	if (frame->depth <= 8) {
		fits_write_img(fptr, dtype, fpixel, nelements, (uint8_t *) frame->image, &status);
	} else if (frame->depth <= 16) {
		fits_write_img(fptr, dtype, fpixel, nelements, (uint16_t *) frame->image, &status);
	} else if (frame->depth <= 32) {
		fits_write_img(fptr, dtype, fpixel, nelements, (uint32_t *) frame->image, &status);
	} else if (frame->depth <= 64) {
		fits_write_img(fptr, dtype, fpixel, nelements, (uint64_t *) frame->image, &status);
	}
	if (status) return status;
	
	// Close file
	fits_close_file(fptr, &status);
	if (status) return status;
	
	return 0;
}

void Camera::calculate_stats(frame_t *const frame) const {
	memset(frame->histo, 0, get_maxval() * sizeof *frame->histo);
	
	if(depth <= 8) {
		uint8_t *image = (uint8_t *)frame->image;
		for(size_t i = 0; i < (size_t) res.x * res.y; i++) {
			frame->histo[image[i]]++;
			if (image[i] > frame->max) frame->max = image[i];
			if (image[i] < frame->min) frame->min = image[i];
		}
	} else {
		uint16_t *image = (uint16_t *)frame->image;
		for(size_t i = 0; i < (size_t) res.x * res.y; i++) {
			frame->histo[image[i]]++;
			if (image[i] > frame->max) frame->max = image[i];
			if (image[i] < frame->min) frame->min = image[i];
		}
	}
	
	double sum = 0;
	double sumsquared = 0;
	
	for(size_t i = 0; i < get_maxval(); i++) {
		sum += (double)i * frame->histo[i];
		sumsquared += (double)(i * i) * frame->histo[i];
	}
	
	sum /= res.x * res.y;
	sumsquared /= res.x * res.y;
	
	frame->avg = sum;
	frame->rms = sqrt(sumsquared - sum * sum) / sum;
}

void *Camera::cam_queue(void * const data, void * const image, struct timeval *const tv) {
	pthread::mutexholder h(&cam_mutex);
	
	frame_t *frame = &frames[count % nframes];
	// Store the old data
	void *old = frame->data;
	// (over)write with new data
	frame->data = data;
	frame->image = image;
	frame->id = count++;

	frame->res = res;
	frame->depth = conv_depth(depth);
	frame->npixels = res.x * res.y;
	// Depth should be ceil'ed to the nearest 8 multiple, because 'depth' could
	// also be 14 or 12 bits in which case 'size' would be wrong.
	//! @todo Need to distinguish between data bitdepth and camera bitdepth
	// Use depth/8, remember depth is in bits, size in bytes
	frame->size = frame->npixels * frame->depth/8;
	
	if(!frame->histo)
		frame->histo = new uint32_t[get_maxval()];
	
	// Reset values in frame struct
	frame->proc = false;
	frame->avg = 0;
	frame->rms = 0;
	frame->min = INT_MAX;
	frame->max = 0;
	
	if(tv)
		frame->tv = *tv;
	else
		gettimeofday(&frame->tv, 0);
	
	{
		pthread::mutexholder h(&proc_mutex);
		proc_cond.signal();			// Signal one waiting thread about the new frame
	}
	
	return old;
}

Camera::frame_t *Camera::get_last_frame() const {
	if(count)
		return &frames[(count - 1) % nframes];
	else
		return 0;
}

Camera::frame_t *Camera::get_next_frame(const bool wait) {
	static size_t frameid = 0;
	
	// Get a newest frame every call, but never get the same frame
	if (frameid == count)
		frameid++;
	else
		frameid = count;
	
	pthread::mutexholder h(&cam_mutex);
	return get_frame(frameid, wait);
}

Camera::frame_t *Camera::get_frame(const size_t id, const bool wait) {	
	if(id >= count) {
		if(wait) {
			while(id >= count) {
				cam_cond.wait(cam_mutex);		// This mutex must be locked elsewhere before calling get_frame()!!
			}
		} else {
			return 0;
		}
	}

	if(id + nframes < count || id >= count)
		return 0;
	
	return &frames[id % nframes];
}

// Network IO starts here
void Camera::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "quit" || command == "exit") {
		conn->write("ok :Bye!");
		conn->close();
	} else if (command == "restart") {
		conn->write("ok");
		do_restart();
	} else if (command == "set") {
		string what = popword(line);
		
		//! @todo what is mode?
		if(what == "mode") {
			string mode_str = popword(line);
			set_mode(str2mode(mode_str));
		} else if(what == "exposure") {
			conn->addtag("exposure");
			set_exposure(popdouble(line));
		} else if(what == "interval") {
			conn->addtag("interval");
			set_interval(popdouble(line));
		} else if(what == "gain") {
			conn->addtag("gain");
			set_gain(popdouble(line));
		} else if(what == "offset") {
			conn->addtag("offset");
			set_offset(popdouble(line));
		} else if(what == "filename") {
			conn->addtag("filename");
			set_filename(popword(line));
		} else if(what == "fits") {
			set_fits(line);
			get_fits(conn);
		} else {
			parsed = false;
			//conn->write("error :Unknown argument " + what);
		}
	} else if (command == "get") {
		string what = popword(line);
		
		if(what == "mode") {
			conn->addtag("mode");
			conn->write("ok mode " + mode2str(mode));
		} else if(what == "exposure") {
			conn->addtag("exposure");
			conn->write(format("ok exposure %lf", exposure));
		} else if(what == "interval") {
			conn->addtag("interval");
			conn->write(format("ok interval %lf", interval));
		} else if(what == "gain") {
			conn->addtag("gain");
			conn->write(format("ok gain %lf", gain));
		} else if(what == "offset") {
			conn->addtag("offset");
			conn->write(format("ok offset %lf", offset));
		} else if(what == "width") {
			conn->write(format("ok width %d", res.x));
		} else if(what == "height") {
			conn->write(format("ok height %d", res.y));
		} else if(what == "depth") {
			conn->write(format("ok depth %d", depth));
		} else if(what == "resolution") {
			conn->write(format("ok resolution %d %d %d", res.x, res.y, depth));
		} else if(what == "filename") {
			conn->addtag("filename");
			conn->write("ok filename :" + filenamebase);
		} else if(what == "fits") {
			get_fits(conn);
		} else {
			parsed = false;
			// conn->write("error :Unknown argument " + what);
		}
	} else if (command == "thumbnail") {
		get_thumbnail(conn);
	} else if (command == "grab") {
		int x1 = popint(line);
		int y1 = popint(line);
		int x2 = popint(line);
		int y2 = popint(line);
		int scale = popint(line);
		bool do_df = false;
		bool do_histo = false;
		string option;
		while(!(option = popword(line)).empty()) {
			if(option == "darkflat")
				do_df = true;
			else if(option == "histogram")
				do_histo = true;
		}
		grab(conn, x1, y1, x2, y2, scale, do_df, do_histo);
	} else if(command == "store") {
		conn->addtag("store");
		set_store(popint(line));
	} else if(command == "dark") {
		if (darkburst(popint(line)) )
			conn->write("error :Error during dark burst");
	} else if(command == "flat") {
		if (flatburst(popint(line)))
			conn->write("error :Error during flat burst");
//	} else if(command == "statistics") {
//		statistics(conn, popint(line));
	} else {
		parsed = false;
		//conn->write("error :Unknown command: " + command);
	}
	
	// If not parsed here, call parent
	if (parsed == false)
		Device::on_message(conn, orig);
}

double Camera::set_exposure(const double value) {	
	cam_set_exposure(value);
	net_broadcast(format("ok exposure %lf", exposure), "exposure");
	set_calib(false);
	return exposure;
}

double Camera::set_interval(const double value) {
	cam_set_interval(value);
	net_broadcast(format("ok interval %lf", interval), "interval");
	return interval;
}

double Camera::set_gain(const double value) {
	cam_set_gain(value);
	net_broadcast(format("ok gain %lf", gain), "gain");
	set_calib(false);
	return gain;
}

double Camera::set_offset(const double value) {
	cam_set_offset(value);
	net_broadcast(format("ok offset %lf", offset), "offset");
	set_calib(false);
	return offset;
}

Camera::mode_t Camera::set_mode(const mode_t value) {
	cam_set_mode(value);
	net_broadcast("ok mode " + mode2str(mode), "mode");
	return mode;
}

int Camera::set_store(const int n) {
	nstore = n;
	net_broadcast(format("ok store %d", nstore), "store");
	return nstore;	
}

void Camera::get_fits(const Connection *const conn = NULL) const {
	if (conn)
		conn->write("ok fits " + fits_observer + ", " + fits_target + ", :" + fits_comments);
}

void Camera::set_fits(string line) {
	set_fits_observer(popword(line, ","));
	set_fits_target(popword(line, ","));
	set_fits_comments(popword(line));
}

string Camera::set_fits_observer(const string val) {
	fits_observer = val;
	return fits_observer;
}

string Camera::set_fits_target(const string val) {
	fits_target = val;
	return fits_target;
}

string Camera::set_fits_comments(const string val) {
	fits_comments = val;
	return fits_comments;
}

string Camera::set_filename(const string value) {
	filenamebase = value;
	net_broadcast("ok filename :" + filenamebase, "filename");
	return filenamebase;
}

Path Camera::makename(const string &base) const {
	struct timeval tv;
	struct tm tm;
	gettimeofday(&tv, 0);
	gmtime_r(&tv.tv_sec, &tm);
	
	Path result = get_outputdir() + format("%s%s_%08d.%06d-%08d.fits", base.c_str(), name.c_str(), tv.tv_sec, tv.tv_usec, count);
	
	return result;
}

uint8_t *Camera::get_thumbnail(Connection *conn = NULL) {
	uint8_t *buffer = new uint8_t[32 * 32];
	uint8_t *out = buffer;
	
	int step, xoff, yoff;
	
	if(res.x < res.y)
		step = res.x / 32;
	else
		step = res.y / 32;
	
	xoff = (res.x - step * 31) / 2;
	yoff = (res.y - step * 31) / 2;
	
	{
		//! @todo Speed issue: Image is locked here, might block camera thread.
		pthread::mutexholder h(&cam_mutex);
		frame_t *f = get_last_frame();
		if(f) {
			if(depth <= 8) {
				uint8_t *in = (uint8_t *)f->image;
				for(int y = 0; y < 32; y++)
					for(int x = 0 ; x < 32; x++)
						*out++ = in[res.x * (yoff + y * step) + xoff + x * step] << (8 - depth);
			} else if(depth <= 16) {
				uint16_t *in = (uint16_t *)f->image;
				for(int y = 0; y < 32; y++)
					for(int x = 0 ; x < 32; x++)
						*out++ = in[res.x * (yoff + y * step) + xoff + x * step] >> (depth - 8);
			}
		}
	}
	
	if (conn) {
		conn->write("ok thumbnail");
		conn->write(buffer, sizeof buffer);
		delete buffer;
		return NULL;
	}
	
	return buffer;
}

void Camera::grab(Connection *conn, int x1, int y1, int x2, int y2, int scale = 1, bool do_df = false, bool do_histo = false) {	
	x1 = clamp(x1, 0, res.x);
	y1 = clamp(y1, 0, res.y);
	x2 = clamp(x2, 0, res.x / scale);
	y2 = clamp(y2, 0, res.y / scale);
//	if(x1 < 0)
//		x1 = 0;
//	if(y1 < 0)
//		y1 = 0;
//	if(scale < 1)
//		scale = 1;
//	//! @todo (int) not necessary here?
//	if(x2 * scale > (int)res.x)
//		x2 = res.x / scale;
//	if(y2 * scale > (int)res.y)
//		y2 = res.y / scale;
//	
//	bool dxt1 = false;
	void *buffer;
	size_t size = (y2 - y1) * (x2 - x1) * (depth <= 8 ? 1 : 2);
//	uint16_t maxval = get_maxval();
//	uint8_t lookup[maxval];
//	for(int i = 0; i < maxval / 2; i++)
//		lookup[i] = pow(i, 7.0 / (depth - 1));
//	for(int i = maxval / 2; i < maxval; i++)
//		lookup[i] = 128 + pow(i - maxval / 2, 7.0 / (depth - 1));
	
	{
		//! @todo locks frame when sending over network?
		pthread::mutexholder h(&cam_mutex);
		frame_t *f = get_frame(count);
		if(!f)
			return conn->write("error :Could not grab image");
		
		string extra;
		
		if(f->tv.tv_sec)
			extra += format(" timestamp %li.%06li", f->tv.tv_sec, f->tv.tv_usec);
		
		if(f->histo && do_histo)
			extra += " histogram";
		
		extra += format(" avg %lf rms %lf", f->avg, f->rms);
		extra += format(" min %d max %d", f->min, f->max);
		
		// zero copy if possible
		if(!do_df && scale == 1 && x1 == 0 && x2 == (int)res.x && y1 == 0 && y2 == (int)res.y) {
			conn->write(format("ok image %zu %d %d %d %d %d", size, x1, y1, x2, y2, scale) + extra);
			conn->write(f->image, size);
			goto finish;
		}
		
		buffer = malloc(size + size/8 + 128);
		
		if(!buffer)
			return conn->write("error :Out of memory");
		
		{
			if(depth <= 8) {
				uint8_t *in = (uint8_t *)f->image;
				uint8_t *p = (uint8_t *)buffer;
				for(size_t y = y1 * scale; y < (size_t) y2 * scale; y += scale)
					for(size_t x = x1 * scale; x < (size_t) x2 * scale; x += scale) {
						size_t o = y * res.x + x;
						if(do_df)
							*p++ = df_correct(in, o);
						else
							*p++ = in[o];
					}
			} else if(depth <= 16) {
				uint16_t *in = (uint16_t *)f->image;
				uint16_t *p = (uint16_t *)buffer;
				for(size_t y = y1 * scale; y < (size_t) y2 * scale; y += scale)
					for(size_t x = x1 * scale; x < (size_t) x2 * scale; x += scale) {
						size_t o = y * res.x + x;
						if(do_df)
							*p++ = df_correct(in, o);
						else
							*p++ = in[o];
					}
			}
						
			conn->write(format("ok image %zu %d %d %d %d %d", size, x1, y1, x2, y2, scale) + extra);
			conn->write(buffer, size);
		}
		
		free(buffer);
		
	finish:
		if(f->histo && do_histo)
			conn->write(f->histo, sizeof *f->histo * get_maxval());
	}
}

uint8_t Camera::df_correct(const uint8_t *in, size_t offset) {
	if (!dark.data || !flat.data)
		return in[offset];
	
	uint32_t *darkim = (uint32_t *) dark.image;
	uint32_t *flatim = (uint32_t *) flat.image;
	
	float c = (in[offset] - darkim[offset]) * flatim[offset];
	if(c < 0)
		c = 0;
	else if(c >= (get_maxval()))
		c = (get_maxval()) - 1;
	return c;
}

uint16_t Camera::df_correct(const uint16_t *in, size_t offset) {
	if (!dark.data || !flat.data)
		return in[offset];

	uint32_t *darkim = (uint32_t *) dark.image;
	uint32_t *flatim = (uint32_t *) flat.image;
	
	float c = (in[offset] - darkim[offset]) * flatim[offset];
	if(c < 0)
		c = 0;
	else if(c >= (get_maxval()))
		c = (get_maxval()) - 1;
	return c;
}

int Camera::darkburst(size_t bcount) {
	// Update dark count
	if (bcount > 0)
		ndark = bcount;
		
	io.msg(IO_DEB1, "Starting dark burst of %zu frames", ndark);
	
	set_mode(RUNNING);
	
	// Allocate memory for darkfield
	uint32_t *accum = new uint32_t[res.x * res.y];
	memset(accum, 0, res.x * res.y * sizeof *accum);

	if(!accumburst(accum, ndark))
		return io.msg(IO_ERR, "Error taking darkframe!");
	
	darkexp = exposure;

	//! @todo implement darkflat save
	//accumsave(dark, "dark", dark_exposure);
	
	// Link data to dark
	if (dark.image) 
		delete (uint32_t*) dark.image;
	
	dark.image = accum;
	dark.data = dark.image;
	
	io.msg(IO_DEB1, "Got new dark.");
	net_broadcast(format("ok dark %d", nflat));
	
	set_mode(OFF);
	return 0;
}

int Camera::flatburst(size_t bcount) {
	// Update flat count
	if (bcount > 0)
		nflat = bcount;
	
	io.msg(IO_DEB1, "Starting flat burst of %zu frames", nflat);
	
	//! @todo fix this
	set_mode(RUNNING);
	//	state = WAITING;
	//	get_state(connection, true);
	//	set_state(WAITING, true);
	
	// Allocate memory for flatfield
	uint32_t *accum = new uint32_t[res.x * res.y];
	memset(accum, 0, res.x * res.y * sizeof *accum);
	
	if(!accumburst(accum, nflat))
		return io.msg(IO_ERR, "Error taking flatframe!");
	
	flatexp = exposure;
	
	//! @todo implement flatflat save
	//accumsave(flat, "flat", flat_exposure);
	
	// Link data to flat
	if (flat.image) 
		delete (uint32_t*) flat.image;
	
	flat.image = accum;
	flat.data = flat.image;
	
	io.msg(IO_DEB1, "Got new flat.");
	net_broadcast(format("ok flat %d", nflat));
	
	set_mode(OFF);
	return 0;
}

bool Camera::accumburst(uint32_t *accum, size_t bcount) {
	pthread::mutexholder h(&cam_mutex);
	
	size_t start = count;
	size_t rx = 0;
	
	while(rx < bcount) {
		frame_t *f = get_frame(start + rx);
		if(!f)
			return false;
		
		if(depth <= 8) {
			uint8_t *image = (uint8_t *)f->image;
			for(size_t i = 0; i < (size_t) res.x * res.y; i++)
				accum[i] += image[i];
		} else {
			uint16_t *image = (uint16_t *)f->image;
			for(size_t i = 0; i < (size_t) res.x * res.y; i++)
				accum[i] += image[i];
		}
		
		rx++;
	}
	
	return true;
}

