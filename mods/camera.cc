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
#include <sys/stat.h>
#ifdef LINUX
#define _XOPEN_SOURCE 600	// for posix_fadvise()
#endif
#include <fcntl.h>

#include "io.h"
#include "types.h"
#include "config.h"
#include "path++.h"

#include "devices.h"
#include "camera.h"

using namespace std;

Camera::Camera(Io &io, foamctrl *ptc, string name, string type, string port, Path &conffile):
Device(io, ptc, name, cam_type + "." + type, port, conffile),
nframes(8), count(0), timeouts(0), ndark(10), nflat(10), 
darkexp(1.0), flatexp(1.0),
interval(1.0), exposure(1.0), gain(1.0), offset(0.0), 
res(0,0), depth(-1), dtype(UINT16),
mode(Camera::OFF),
filenamebase("FOAM"), outputdir(ptc->datadir), nstore(0),
fits_telescope("undef"), fits_observer("undef"), fits_instrument("undef"), fits_target("undef"), fits_comments("undef")
{
	io.msg(IO_DEB2, "Camera::Camera()");
	
	// Set buffer size (default 8 frames)
	nframes = cfg.getint("nframes", 8);
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
	res.x = cfg.getint("width", 512);
	res.y = cfg.getint("height", 512);
	depth = cfg.getint("depth", 8);

	proc_thr.create(sigc::mem_fun(*this, &Camera::cam_proc));
}

Camera::~Camera() {
	io.msg(IO_DEB2, "Camera::~Camera()");
	proc_thr.cancel();
	proc_thr.join();
	delete[] frames;
}

void Camera::cam_proc() {
	io.msg(IO_DEB2, "Camera::cam_proc()");
	frame_t *frame;
	
	while (true) {
		// Always wait for proc_cond broadcasts
		proc_mutex.lock();
		io.msg(IO_DEB2, "Camera::cam_proc(): waiting for proc_cond.");
		proc_cond.wait(proc_mutex);
		proc_mutex.unlock();
		
		// Lock cam_mut before handling the data
		io.msg(IO_DEB2, "Camera::cam_proc(): waiting for cam_mutex.");
		pthread::mutexholder h(&cam_mutex);
		
		// There is a new frame ready now, process it
		frame = get_last_frame();
		
		calculate_stats(frame);

		if (nstore == -1 || nstore > 0) {
			nstore--;
			if (store_frame(frame))
				netio.broadcast(format("ok store %d", nstore), "store");
		}
		
		// Flag frame as processed
		frame->proc = true;
		
		// Notify all threads waiting for new frames now
		cam_cond.broadcast();
	}
	io.msg(IO_DEB2, "Camera::cam_proc() finish");
}

void Camera::fits_init_phdu(char *phdu) {
	memset(phdu, ' ', 2880);
}

bool Camera::fits_add_card(char *phdu, const string &key, const string &value) {
	int i;
	
	for(i = 0; i < 36; i++) {
		if(*phdu == ' ' || *phdu == '\0')
			break;
		phdu += 80;
	}
	
	if(i >= 36)
		return false;
	
	memcpy(phdu, key.data(), key.size() < 8 ? key.size() : 8);
	
	if(value.size()) {
		phdu[8] = '=';
		memcpy(phdu + 10, value.data(), value.size() < 70 ? value.size() : 70);
	}
	
	return true;
}

bool Camera::fits_add_comment(char *phdu, const string &comment) {
	int i;
	
	for(i = 0; i < 35; i++) {
		if(*phdu == ' ' || *phdu == '\0')
			break;
		phdu += 80;
	}
	
	if(i >= 35)
		return false;
	
	memcpy(phdu, "COMMENT", 7);
	memcpy(phdu + 10, comment.data(), comment.size() < 70 ? comment.size() : 70);
	
	return true;
}


bool Camera::store_frame(frame_t *frame) {
	// Generate path to store file to, based on filenamebase
	Path filename = makename();
	io.msg(IO_DEB1, "Camera::store_frame(%p) to %s", frame, filename.c_str());
	
	// Open file
	int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd < 0)
		return false;
	
	// Get time & date
	struct timeval tv;
	struct tm tm;
	gettimeofday(&tv, 0);
	gmtime_r(&tv.tv_sec, &tm);
	string date = format("%04d-%02d-%02dT%02d:%02d:%02d", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	// Init FITS header unit
	char phdu[2880];
	
	fits_init_phdu(phdu);
	
	// Add datatype <http://www.eso.org/sci/data-processing/software/esomidas//doc/user/98NOV/vola/node112.html>
	fits_add_card(phdu, "SIMPLE", "T");
	if (dtype == UINT8 || dtype == INT8)
		fits_add_card(phdu, "BITPIX", "8");
	else if (dtype == UINT16 || dtype == INT16)
		fits_add_card(phdu, "BITPIX", "16");
	else if (dtype == FLOAT32)
		fits_add_card(phdu, "BITPIX", "-32");
	else if (dtype == FLOAT64)
		fits_add_card(phdu, "BITPIX", "-64");
		
	// Add rest of the metadata
	fits_add_card(phdu, "NAXIS", "2");
	fits_add_card(phdu, "NAXIS1", format("%d", res.x));
	fits_add_card(phdu, "NAXIS2", format("%d", res.y));
	
	fits_add_card(phdu, "ORIGIN", "FOAM store");
	fits_add_card(phdu, "DEVICE", name);
	fits_add_card(phdu, "TELESCOPE", fits_telescope);
	fits_add_card(phdu, "INSTRUMENT", fits_instrument);
	fits_add_card(phdu, "DATE", date);
	fits_add_card(phdu, "OBSERVER", fits_observer);
	fits_add_card(phdu, "EXPTIME", format("%lf / [s]", exposure));
	fits_add_card(phdu, "INTERVAL", format("%lf / [s]", interval));
	fits_add_card(phdu, "GAIN", format("%lf", gain));
	fits_add_card(phdu, "OFFSET", format("%lf", offset));

	string comments = fits_comments;
	
	while(true) {
		string comment = popword(comments, ";");
		if(comment.empty())
			break;
		fits_add_comment(phdu, comment);
	}
	
	fits_add_card(phdu, "END", "");
	
#ifdef LINUX
	posix_fadvise(fd, 0, (res.x * res.y * depth/8) + sizeof phdu, POSIX_FADV_DONTNEED); 
#endif
	write(fd, phdu, sizeof phdu);
	write(fd, frame->image, res.x * res.y * depth/8);
	close(fd);
	
	return true;
}

void Camera::calculate_stats(frame_t *frame) {
	memset(frame->histo, 0, get_maxval() * sizeof *frame->histo);
	
	if(depth <= 8) {
		uint8_t *image = (uint8_t *)frame->image;
		for(size_t i = 0; i < (size_t) res.x * res.y; i++) {
			frame->histo[image[i]]++;
		}
	} else {
		uint16_t *image = (uint16_t *)frame->image;
		for(size_t i = 0; i < (size_t) res.x * res.y; i++) {
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
	pthread::mutexholder h(&cam_mutex);
	
	frame_t *frame = &frames[count % nframes];
	// Store the old data
	void *old = frame->data;
	// (over)write with new data
	frame->data = data;
	frame->image = image;
	frame->id = count++;
	
	if(!frame->histo)
		frame->histo = new uint32_t[get_maxval()];
			
	if(tv)
		frame->tv = *tv;
	else
		gettimeofday(&frame->tv, 0);
	
	// Signal one waiting thread about the new frame
	proc_cond.signal();
	
	return old;
}

Camera::frame_t *Camera::get_last_frame() {
	if(count)
		return &frames[(count - 1) % nframes];
	else
		return 0;
}

Camera::frame_t *Camera::get_frame(size_t id, bool wait) {	
	if(id >= count) {
		if(wait) {
			while(id >= count)
				cam_cond.wait(cam_mutex);		// This mutex must be locked elsewhere before calling get_frame()
		} else {
			return 0;
		}
	}
	
	if(id < count - nframes || id >= count)
		return 0;
	
	return &frames[id % nframes];
}

// Network IO starts here
void Camera::on_message(Connection *conn, string line) {
	io.msg(IO_DEB2, "Camera::on_message(line=%s)", line.c_str());
	
	string command = popword(line);
	
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
		} else if(what == "outputdir") {
			conn->addtag("outputdir");
			string dir = popword(line);
			if (set_outputdir(dir) == "")
				conn->write("error :directory "+dir+" not usable");
		} else if(what == "fits") {
			set_fits(line);
			get_fits(conn);
		} else
			conn->write("error :Unknown argument " + what);
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
		} else if(what == "filename") {
			conn->addtag("filename");
			conn->write("ok filename :" + filenamebase);
		} else if(what == "outputdir") {
			conn->addtag("outputdir");
			conn->write("ok outputdir :" + outputdir.str());
		} else if(what == "fits") {
			get_fits(conn);
		} else {
			conn->write("error :Unknown argument " + what);
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
	} else if(command == "statistics") {
		statistics(conn, popint(line));
	} else {
		conn->write("error :Unknown command: " + command);
	}
}

double Camera::set_exposure(double value) {	
	cam_set_exposure(value);
	accumfix();
	netio.broadcast(format("ok exposure %lf", exposure), "exposure");
	return exposure;
}

double Camera::set_interval(double value) {
	cam_set_interval(value);
	netio.broadcast(format("ok interval %lf", interval), "interval");
	return interval;
}

double Camera::set_gain(double value) {
	cam_set_gain(value);
	netio.broadcast(format("ok gain %lf", gain), "gain");
	return gain;
}

double Camera::set_offset(double value) {
	cam_set_offset(value);
	netio.broadcast(format("ok offset %lf", offset), "offset");
	return offset;
}

Camera::mode_t Camera::set_mode(mode_t value) {
	cam_set_mode(value);
	netio.broadcast("ok mode " + mode2str(mode), "mode");
	return mode;
}

int Camera::set_store(int n) {
	nstore = n;
	netio.broadcast(format("ok store %d", nstore), "store");
	return nstore;	
}

void Camera::get_fits(Connection *conn = NULL) {
	if (conn)
		conn->write("ok fits " + fits_observer + ", " + fits_target + ", :" + fits_comments);
}

void Camera::set_fits(string line) {
	set_fits_observer(popword(line, ","));
	set_fits_target(popword(line, ","));
	set_fits_comments(popword(line));
}

string Camera::set_fits_observer(string val) {
	fits_observer = val;
	return fits_observer;
}

string Camera::set_fits_target(string val) {
	fits_target = val;
	return fits_target;
}

string Camera::set_fits_comments(string val) {
	fits_comments = val;
	return fits_comments;
}

string Camera::set_filename(string value) {
	filenamebase = value;
	netio.broadcast("ok filename :" + filenamebase, "filename");
	return filenamebase;
}

string Camera::set_outputdir(string value) {
	// This automatically prefixes ptc->datadir if 'value' is not absolute
	Path tmp = ptc->datadir + value;
	
	if (!tmp.exists()) // Does not exist, create
		if (mkdir(value.c_str(), 0755)) // mkdir() returned !0, error
			return "";
	else	// Directory exists, check if readable
		if (tmp.access(R_OK | W_OK | X_OK))
			return "";
	
	outputdir = tmp;
	netio.broadcast("ok outputdir :" + outputdir.str(), "outputdir");
	return outputdir.str();
}

Path Camera::makename(const string &base) {
	struct timeval tv;
	struct tm tm;
	gettimeofday(&tv, 0);
	gmtime_r(&tv.tv_sec, &tm);
	
	Path result = outputdir + format("%4d-%02d-%02d/", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday);
	mkdir(result.c_str(), 0755);
	
	result += (base + format("_%s_%08d.%06d-%08d.fits", name.c_str(), tv.tv_sec, tv.tv_usec, count));
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
			conn->write(f->histo, sizeof *f->histo * maxval);
	}
}

const uint8_t Camera::df_correct(const uint8_t *in, size_t offset) {
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

const uint16_t Camera::df_correct(const uint16_t *in, size_t offset) {
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


void Camera::accumfix() {
	io.msg(IO_DEB2, "Camera::accumfix()");
	if (exposure != darkexp) {
//		uint32_t *darkim = (uint32_t *) dark.image;
//		for(size_t i = 0; i < res.x * res.y; i++)
//			darkim[i] = 0;
		dark.data = NULL;
		darkexp = exposure;
	}
	
	if (exposure != flatexp) {
//		uint32_t *flatim = (uint32_t *) flat.image;
//		for(size_t i = 0; i < res.x * res.y; i++)
//			flatim[i] = 1.0;
		flat.data = NULL;
		flatexp = exposure;
	}
}

int Camera::darkburst(size_t bcount) {
	// Update dark count
	if (bcount > 0)
		ndark = bcount;
		
	io.msg(IO_DEB1, "Starting dark burst of %zu frames", ndark);
	
	//! @todo fix this
	set_mode(RUNNING);
//	state = WAITING;
//	get_state(connection, true);
//	set_state(WAITING, true);
	
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
	netio.broadcast(format("ok dark %d", nflat));
	
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
	netio.broadcast(format("ok flat %d", nflat));
	
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

void Camera::statistics(Connection *conn, size_t bcount) {
	if (bcount < 1)
		bcount = 1;
	
	double avg = 0;
	double rms = 0;
	size_t rx = 0;
	
	{
		pthread::mutexholder h(&cam_mutex);
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
	
	conn->write(format("ok statistics %lf %lf", avg, rms));
}
