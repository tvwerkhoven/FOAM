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

#include "io.h"
#include "types.h"
#include "config.h"
#include "path++.h"

#include "devices.h"
#include "camera.h"

using namespace std;

Camera::Camera(Io &io, foamctrl *const ptc, const string name, const string type, const string port, Path const &conffile, const bool online):
Device(io, ptc, name, cam_type + "." + type, port, conffile, online),
nframes(8), count(0), timeouts(0), ndark(10), nflat(10), 
darkexp(1.0), flatexp(1.0),
interval(1.0), exposure(1.0), gain(1.0), offset(0.0), 
res(0,0), depth(-1),
mode(Camera::OFF),
filenamebase("FOAM"), outputdir(ptc->datadir), nstore(0),
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
	add_cmd("set outputdir");
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
	add_cmd("get outputdir");
	add_cmd("get fits");
	add_cmd("thumbnail");
	add_cmd("grab");
	add_cmd("store");
	add_cmd("dark");
	add_cmd("flat");
//	add_cmd("statistics");
	
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
	res.y = cfg.getint("height", 512);
	res.x = cfg.getint("width", 768);
	depth = cfg.getint("depth", 8);

	io.msg(IO_XNFO, "Camera::Camera(): %dx%dx%d, exp:%g, int:%g, gain:%g, off:%g",
				 res.x, res.y, depth, exposure, interval, gain, offset);
	
	proc_thr.create(sigc::mem_fun(*this, &Camera::cam_proc));
}

Camera::~Camera() {
	io.msg(IO_DEB2, "Camera::~Camera()");
	proc_thr.cancel();
	proc_thr.join();
	
	// frames themselves should be free()'ed by derived classes, they know how to
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
		
		// Lock cam_mut before handling the data
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
}

void Camera::fits_init_phdu(char *const phdu) const {
	memset(phdu, ' ', 2880);
}

bool Camera::fits_add_card(char *phdu, const string &key, const string &value) const {
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

bool Camera::fits_add_comment(char *phdu, const string &comment) const {
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

bool Camera::store_frame(const frame_t *const frame) const {
	// Generate path to store file to, based on filenamebase
	if (depth != 8 && depth != 16) {
		io.msg(IO_WARN, "Camera::store_frame() Only 8 and 16 bit images supported!");
		return false;
	}
	
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
	if (depth == 16)
		fits_add_card(phdu, "BITPIX", "16");
	else
		fits_add_card(phdu, "BITPIX", "8");
		
	// Add rest of the metadata
	fits_add_card(phdu, "NAXIS", "2");
	fits_add_card(phdu, "NAXIS1", format("%d", res.x));
	fits_add_card(phdu, "NAXIS2", format("%d", res.y));
	
	fits_add_card(phdu, "ORIGIN", "FOAM Camera");
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
	ssize_t ret = write(fd, phdu, sizeof phdu);
	ret += write(fd, frame->image, res.x * res.y * depth/8);
	close(fd);
	if (ret != (ssize_t) sizeof phdu + (res.x * res.y * depth/8))
		return io.msg(IO_ERR, "Camera::store_frame() Writing failed!");
	
	return true;
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
	frame->size = res.x * res.y * depth/8;
	
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
	
	proc_cond.signal();			// Signal one waiting thread about the new frame
	
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
	
	return get_frame(frameid, wait);
}

Camera::frame_t *Camera::get_frame(const size_t id, const bool wait) {	
	if(id >= count) {
		if(wait) {
			while(id >= count) {
				cam_cond.wait(cam_mutex);		// This mutex must be locked elsewhere before calling get_frame()
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
		} else if(what == "outputdir") {
			conn->addtag("outputdir");
			string dir = popword(line);
			if (set_outputdir(dir) == "")
				conn->write("error :directory "+dir+" not usable");
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
		} else if(what == "outputdir") {
			conn->addtag("outputdir");
			conn->write("ok outputdir :" + outputdir.str());
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
	netio.broadcast(format("ok exposure %lf", exposure), "exposure");
	set_calib(false);
	return exposure;
}

double Camera::set_interval(const double value) {
	cam_set_interval(value);
	netio.broadcast(format("ok interval %lf", interval), "interval");
	return interval;
}

double Camera::set_gain(const double value) {
	cam_set_gain(value);
	netio.broadcast(format("ok gain %lf", gain), "gain");
	set_calib(false);
	return gain;
}

double Camera::set_offset(const double value) {
	cam_set_offset(value);
	netio.broadcast(format("ok offset %lf", offset), "offset");
	set_calib(false);
	return offset;
}

Camera::mode_t Camera::set_mode(const mode_t value) {
	cam_set_mode(value);
	netio.broadcast("ok mode " + mode2str(mode), "mode");
	set_calib(false);
	return mode;
}

int Camera::set_store(const int n) {
	nstore = n;
	netio.broadcast(format("ok store %d", nstore), "store");
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
	netio.broadcast("ok filename :" + filenamebase, "filename");
	return filenamebase;
}

string Camera::set_outputdir(const string value) {
	// This automatically prefixes ptc->datadir if 'value' is not absolute
	Path tmp = ptc->datadir + value;
	
	if (!tmp.exists()) {// Does not exist, create
		if (mkdir(value.c_str(), 0755)) // mkdir() returned !0, error
			return "";
	}
	else {	// Directory exists, check if readable
		if (tmp.access(R_OK | W_OK | X_OK))
			return "";
	}
	
	outputdir = tmp;
	netio.broadcast("ok outputdir :" + outputdir.str(), "outputdir");
	return outputdir.str();
}

Path Camera::makename(const string &base) const {
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
//	
//	bool dxt1 = false;
	void *buffer;
	size_t size = (y2 - y1) * (x2 - x1) * (depth <= 8 ? 1 : 2);
	uint16_t maxval = get_maxval();
	uint8_t lookup[maxval];
	for(int i = 0; i < maxval / 2; i++)
		lookup[i] = pow(i, 7.0 / (depth - 1));
	for(int i = maxval / 2; i < maxval; i++)
		lookup[i] = 128 + pow(i - maxval / 2, 7.0 / (depth - 1));
	
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
			conn->write(f->histo, sizeof *f->histo * maxval);
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

