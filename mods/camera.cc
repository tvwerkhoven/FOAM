/*
 cam.cc -- generic camera input/output wrapper
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
 @file cam.cc
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl) and Guus Sliepen (guus@sliepen.org)
 @brief Generic camera class.
 
 This class extends the Device class and provides a base class for cameras. Does not implement anything itself.
 */

#include <string>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>

#include "io.h"
#include "types.h"
#include "config.h"

#include "devices.h"
#include "camera.h"

using namespace std;

Camera::Camera(Io &io, foamctrl *ptc, string name, string type, string port, string conffile): 
Device(io, ptc, name, cam_type + "." + type, port, conffile),
nframes(8), timeouts(0), ndark(10), nflat(10), 
interval(1.0), exposure(1.0), gain(1.0), offset(0.0), 
res(0,0), depth(-1), dtype(DATA_UINT16),
mode(Camera::OFF)
{
	io.msg(IO_DEB2, "Camera::Camera()");
	
	nframes = cfg.getint("nframes", 8);
	frames = new frame_t[nframes];
	
	ndark = cfg.getint("ndark", 10);
	nflat = cfg.getint("nflat", 10);
	
	interval = cfg.getdouble("interval", 1.0);
	exposure = cfg.getdouble("exposure", 1.0);
	gain = cfg.getdouble("gain", 1.0);
	offset = cfg.getdouble("offset", 0.0);
	
	res.x = cfg.getint("width", 512);
	res.y = cfg.getint("height", 512);
	depth = cfg.getint("depth", 8);
	
}

Camera::~Camera() {
	delete frames;
}

void Camera::calculate_stats(frame_t *frame) {
	memset(frame->histo, 0, get_maxval() * sizeof *frame->histo);
	
	if(depth <= 8) {
		uint8_t *image = (uint8_t *)frame->image;
		for(size_t i = 0; i < res.x * res.y; i++) {
			frame->histo[image[i]]++;
		}
	} else {
		uint16_t *image = (uint16_t *)frame->image;
		for(size_t i = 0; i < res.x * res.y; i++) {
			frame->histo[image[i]]++;
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

void *Camera::cam_queue(void *data, void *image, struct timeval *tv) {
	pthread::mutexholder h(&mutex);
	
	frame_t *frame = &frames[count % nframes];
	void *old = frame->data;
	frame->data = data;
	frame->image = image;
	frame->id = count++;
	
	if(!frame->histo)
		frame->histo = new uint32_t[maxval];
	
	calculate_stats(frame);
	
	//! @todo Could still implement this (partially)
	frame->rms1 = frame->rms2 = 0.0/0.0;
	frame->cx = frame->cy = frame->cr = 0.0/0.0;
	frame->dx = frame->dy = 0.0/0.0;
	
	if(tv)
		frame->tv = *tv;
	else
		gettimeofday(&frame->tv, 0);
	io.msg(IO_DEB1, "\r%8zu %p %p %p %7.3lf %6.3lf", count, frame, data, image, frame->avg, frame->rms);
	
	
	cam_cond.broadcast();
	
	return old;
}

static frame_t *Camera::get_frame(size_t id, bool wait = true) {
	if(id >= count) {
		if(wait) {
			//! @todo Shouldn't we lock the mutex here first?
			while(id >= count)
				cam_cond.wait(cam_mutex);
		} else {
			return 0;
		}
	}
	
	if(id < count - nframes || id >= count)
		return 0;
	
	return &frames[id % nframes];
}

// Network IO starts here

void Camera::on_message(conn *conn, string line) {
	string command = popword(line);
	
	if (command == "quit" || command == "exit") {
		conn->write("OK :Bye!");
		conn->close();
	} else if (command == "restart") {
		conn->write("OK");
		do_restart();
	} else if (command == "set") {
		string what = popword(line);
		
		//! @todo what is mode?
		if(what == "mode")
			set_mode(conn, popword(line));
		else if(what == "exposure")
			set_exposure(conn, popdouble(line));
		else if(what == "interval")
			set_interval(conn, popdouble(line));
		else if(what == "gain")
			set_gain(conn, popdouble(line));
		else if(what == "offset")
			set_offset(conn, popdouble(line));
		else if(what == "filename")
			set_filename(conn, popword(line));
		else if(what == "outputdir")
			set_outputdir(conn, popword(line));
		else if(what == "fits")
			set_fits(conn, line);
		else
			conn->write("ERROR :Unknown argument " + what);
	} else if (command == "get") {
		string what = popword(line);
		
		if(what == "mode") {
			conn->addtag("mode");
			get_mode(conn);
		} else if(what == "exposure") {
			conn->addtag("exposure");
			conn->write(format("OK exposure %lf", exposure));
		} else if(what == "interval") {
			conn->addtag("interval");
			conn->write(format("OK interval %lf", interval));
		} else if(what == "gain") {
			conn->addtag("gain");
			conn->write(format("OK gain %lf", gain));
		} else if(what == "offset") {
			conn->addtag("offset");
			conn->write(format("OK offset %lf", offset));
		} else if(what == "width") {
			conn->write(format("OK width %d", width));
		} else if(what == "height") {
			conn->write(format("OK height %d", height));
		} else if(what == "depth") {
			conn->write(format("OK depth %d", depth));
		} else if(what == "state") {
			conn->addtag("state");
			get_state(conn);
		} else if(what == "filename") {
			conn->addtag("filename");
			conn->write("OK filename :" + filenamebase);
		} else if(what == "outputdir") {
			conn->addtag("outputdir");
			conn->write("OK outputdir :" + outputdir);
		} else if(what == "fits") {
			get_fits(conn);
		} else {
			conn->write("ERROR :Unknown argument " + what);
		}
	} else if (command == "thumbnail") {
		thumbnail(conn);
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
	} else if(command == "dark") {
		darkburst(conn, popint(line));
	} else if(command == "flat") {
		flatburst(conn, popint(line));
	} else if(command == "statistics") {
		statistics(conn, popint(line));
	} else {
		conn->write("ERROR :Unknown command");
	}
}

void Camera::set_exposure(Connection *conn, double value) {
	cam_set_exposure(value);
	accumfix();
	conn->addtag("exposure");
	netio.broadcast(format("OK exposure %lf", exposure), "exposure");
}

void Camera::set_interval(Connection *conn, double value) {
	cam_set_interval(value);
	conn->addtag("interval");
	netio.broadcast(format("OK interval %lf", interval), "interval");
}

void Camera::set_gain(Connection *conn, double value) {
	cam_set_gain(value);
	conn->addtag("gain");
	netio.broadcast(format("OK gain %lf", gain), "gain");
}

void Camera::set_offset(Connection *conn, double value) {
	cam_set_offset(value);
	conn->addtag("offset");
	netio.broadcast(format("OK offset %lf", offset), "offset");
}

void Camera::set_mode(Connection *conn, string value) {
	get_mode(conn, true);
}

void Camera::get_fits(Connection *conn) {
	conn->write("OK fits " + fits_observer + ", " + fits_target + ", :" + fits_comments);
}

void Camera::set_fits(Connection *conn, string line) {
	fits_observer = popword(line, ",");
	fits_target = popword(line, ",");
	fits_comments = popword(line);
	get_fits(conn);
}

void Camera::set_filename(Connection *conn, string value) {
	filenamebase = value;
	conn->addtag("filename");
	netio.broadcast("OK filename :" + filenamebase, "filename");
}

void Camera::set_outputdir(Connection *conn, string value) {
	// If it's not an absolute path (starting with '/'), prefix ptc->datadir
	if (value.substr(0,1) != "/")
		value = ptc->datadir + "/" + value;
		
	if(access(value.c_str(), R_OK | W_OK | X_OK)) {
		conn->write("ERROR :directory not usable");
		return;
	}
	
	outputdir = value;
	conn->addtag("outputdir");
	netio.broadcast("OK outputdir :" + outputdir, "outputdir");
}

string Camera::makename(const string &base = filenamebase) {
	struct timeval tv;
	struct tm tm;
	gettimeofday(&tv, 0);
	gmtime_r(&tv.tv_sec, &tm);
	
	string result = outputdir + format("/%4d-%02d-%02d/", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday);
	mkdir(result.c_str(), 0755);
	result += base + format("_%08d_%s.fits", tv.tv_sec, id.c_str());
	return result;
}

void Camera::thumbnail(Connection *conn) {
	uint8_t buffer[32 * 32];
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
						*out++ = in[width * (yoff + y * step) + xoff + x * step] << (8 - depth);
			} else if(depth <= 16) {
				uint16_t *in = (uint16_t *)f->image;
				for(int y = 0; y < 32; y++)
					for(int x = 0 ; x < 32; x++)
						*out++ = in[width * (yoff + y * step) + xoff + x * step] >> (depth - 8);
			}
		}
	}
	
	conn->write("OK thumbnail");
	conn->write(buffer, sizeof buffer);
}


template <class T> static T clamp(T x, T min, T max) { if (x < min) x = min; if (x > max) x = max; return x; }

static void Camera::grab(Connection *conn, int x1, int y1, int x2, int y2, int scale = 1, bool do_df = false, bool do_histo = false) {
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
	
	bool dxt1 = false;
	void *buffer;
	size_t size = (y2 - y1) * (x2 - x1) * (depth <= 8 ? 1 : 2);
	uint16_t maxval = get_maxval();
	uint8_t lookup[maxval];
	for(int i = 0; i < maxval / 2; i++)
		lookup[i] = pow(i, 7.0 / (depth - 1));
	for(int i = maxval / 2; i < maxval; i++)
		lookup[i] = 128 + pow(i - maxval / 2, 7.0 / (depth - 1));
	
	{
		pthread::mutexholder h(&mutex);
		frame_t *f = get_frame(count);
		if(!f)
			return conn->write("ERROR :Could not grab image");
		
		string extra;
		
		if(f->tv.tv_sec)
			extra += format(" timestamp %li.%06li", f->tv.tv_sec, f->tv.tv_usec);
		
		if(f->histo && do_histo)
			extra += " histogram";
		
		extra += format(" avg %lf rms %lf", f->avg, f->rms);
		
		// zero copy if possible
		if(!do_df && scale == 1 && x1 == 0 && x2 == (int)res.x && y1 == 0 && y2 == (int)res.y) {
			connection->write(format("OK image %zu %d %d %d %d %d", size, x1, y1, x2, y2, scale) + extra);
			connection->write(f->image, size);
			goto finish;
		}
		
		buffer = malloc(size + size/8 + 128);
		
		if(!buffer)
			return conn->write("ERROR :Out of memory");
		
		{
			if(depth <= 8) {
				uint8_t *in = (uint8_t *)f->image;
				uint8_t *p = (uint8_t *)buffer;
				for(size_t y = y1 * scale; y < y2 * scale; y += scale)
					for(size_t x = x1 * scale; x < x2 * scale; x += scale) {
						size_t o = y * res.x + x;
						if(do_df)
							*p++ = df_correct(in, o);
						else
							*p++ = in[o];
					}
			} else if(depth <= 16) {
				uint16_t *in = (uint16_t *)f->image;
				uint16_t *p = (uint16_t *)buffer;
				for(size_t y = y1 * scale; y < y2 * scale; y += scale)
					for(size_t x = x1 * scale; x < x2 * scale; x += scale) {
						size_t o = y * res.x + x;
						if(df)
							*p++ = df_correct(in, o);
						else
							*p++ = in[o];
					}
			}
			
			conn->write(format("OK image %zu %d %d %d %d %d", size, x1, y1, x2, y2, scale) + extra);
			conn->write(buffer, size);
		}
		
		free(buffer);
		
	finish:
		if(f->histo && do_histo)
			connection->write(f->histo, sizeof *f->histo * maxval);
	}
}

static void Camera::accumfix() {
	if (exposure != darkexp) {
		for(size_t i = 0; i < res.x * res.y; i++)
			dark.image[i] = 0;
		dark.data = NULL;
		darkexp = exposure;
	}
	
	if (exposure != flatexp) {
		for(size_t i = 0; i < res.x * res.y; i++)
			flat.image[i] = 1.0;
		flat.data = NULL;
		flatexp = exposure;
	}
}

static void Camera::darkburst(Connection *conn, size_t bcount) {
	// Update dark count
	if (bcount > 0)
		ndark = bcount;
		
	io.msg(IO_DEB1, "Starting dark burst of %zu frames", ndark);
	
	//! @todo fix this
	state = WAITING;
	get_state(connection, true);
	set_state(WAITING, true);
	
	// Allocate memory for darkfield
	uint32_t *accum = new uint32_t[res.x * res.y];
	memset(accum, 0, res.x * res.y * sizeof *accum);

	if(!accumburst(accum, ndark))
		conn->write("ERROR :Error during dark burst");
	else {
		darkexp = exposure;
	}
	
	//accumsave(dark, "dark", dark_exposure);
	
	// Link data to dark
	if (dark.image) 
		delete dark.image;
	
	dark.image = accum;
	dark.data = dark.image;
	
	io.msg(IO_DEB1, "Got new dark.");
	
	state = READY;
	get_state(connection, true);
}

static void Camera::flatburst(Connection *conn, size_t bcount) {
	// Update flat count
	if (bcount > 0)
		nflat = bcount;
	
	io.msg(IO_DEB1, "Starting flat burst of %zu frames", nflat);
	
	//! @todo fix this
	state = WAITING;
	get_state(connection, true);
	set_state(WAITING, true);
	
	// Allocate memory for flatfield
	uint32_t *accum = new uint32_t[res.x * res.y];
	memset(accum, 0, res.x * res.y * sizeof *accum);
	
	if(!accumburst(accum, nflat))
		conn->write("ERROR :Error during flat burst");
	else {
		flatexp = exposure;
	}
	
	//accumsave(flat, "flat", flat_exposure);
	
	// Link data to flat
	if (flat.image) 
		delete flat.image;
	
	flat.image = accum;
	flat.data = flat.image;
	
	io.msg(IO_DEB1, "Got new flat.");
	
	state = READY;
	get_state(connection, true);
}

static bool Camera::accumburst(uint32_t *accum, size_t bcount) {
	pthread::mutexholder h(&mutex);
	
	size_t start = count;
	size_t rx = 0;
	
	while(rx < bcount) {
		frame_t *f = get_frame(start + rx);
		if(!f)
			return false;
		
		if(depth <= 8) {
			uint8_t *image = (uint8_t *)f->image;
			for(size_t i = 0; i < res.x * res.y; i++)
				accum[i] += image[i];
		} else {
			uint16_t *image = (uint16_t *)f->image;
			for(size_t i = 0; i < res.x * res.y; i++)
				accum[i] += image[i];
		}
		
		rx++;
	}
	
	return true;
}

static void Camera::statistics(Connection *conn, size_t bcount) {
	if (bcount < 1)
		bcount = 1;
	
	double avg = 0;
	double rms = 0;
	size_t rx = 0;
	
	{
		pthread::mutexholder h(&mutex);
		size_t start = count;
		
		while(rx < bcount) {
			frame_t *f = get_frame(start + rx);
			if(!f)
				break;
			
			avg += f->avg;
			rms += f->rms * f->rms;
			
			rx++;
		}
	}
	
	avg /= rx;
	rms /= rx;
	rms = sqrt(rms);
	
	conn->write(format("OK statistics %lf %lf", avg, rms));
}
