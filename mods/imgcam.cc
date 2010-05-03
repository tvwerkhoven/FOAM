/*
 imgcam.cc -- Dummy 'camera' with static images as source
 Copyright (C) 2009--2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <sys/time.h>
#include <time.h>
#include <math.h>
// TODO: compiling fails when this is removed, warns when present
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "config.h"
#include "io.h"

#include "imgio.h"
#include "imgcam.h"

void ImgCamera::update(bool blocking) {
	// Copy image from img to frame and add some noise etc
	io.msg(IO_DEB2, "ImgCamera::update()");
	if (blocking)
		usleep((int) interval * 1000000);
	
	// Shortcut to image, casted to right type.
	uint16_t *p = (uint16_t *) image;
	
	for(int y = 0; y < res.y; y++) {
		for(int x = 0; x < res.x; x++) {
			double value = drand48() * noise + img->getPixel(x, y) * exposure;
			if (value < 0)
				value = 0;
			if (value > UINT16_MAX)
				value = UINT16_MAX;
			*p++ = (uint16_t)(value);
		}
	}
}

ImgCamera::ImgCamera(Io &io, string name, string port, config *config): 
Camera(io, name, port) {
	io.msg(IO_DEB2, "ImgCamera::ImgCamera()");
	
	string type = config->getstring(name+".type");
	if (type != IMGCAM_TYPE) throw exception("Type should be '" IMGCAM_TYPE "' for this class.");
	
	string file = config->getstring(name+".imagefile");
	if (file[0] != '/') file = FOAM_DATADIR "/" + file;
	
	io.msg(IO_DEB2, "imagefile = %s", file.c_str());
	noise = config->getdouble(name+".noise", 10.0);
	interval = config->getdouble(name+".interval", 0.25);
	exposure = config->getdouble(name+".exposure", 1.0);
	
	mode = Camera::OFF;
	
	img = new Imgio(io, file, Imgio::FITS);
	img->loadImg();
	
	res.x = img->getWidth();
	res.y = img->getHeight();
	bpp = 16;
	dtype = DATA_UINT16;
	
	image = (void *) malloc(res.x * res.y * 2);
	
	if (!image)
		throw exception("Could not allocate memory for framebuffer");
	
	update(true);
	
	io.msg(IO_INFO, "ImgCamera init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
				 res.x, res.y, bpp, noise, interval, exposure);
	io.msg(IO_INFO, "Range = %d--%d, sum=%lld", img->range[0], img->range[1], img->sum);
}

ImgCamera::~ImgCamera() {
	io.msg(IO_DEB2, "ImgCamera::~ImgCamera()");
	delete img;
}

int ImgCamera::verify() { 
	// Verify some camera settings
	if (res.x <= 0 || 
			res.y <= 0 ||
			(bpp != 8 || bpp != 16)) return 1;
	
	return 0;
}

void ImgCamera::on_message(Connection *connection, std::string line) {
	io.msg(IO_DEB2, "ImgCamera::on_message(Connection *connection, std::string line)");
	
	// Process everything in uppercase
	transform(line.begin(), line.end(), line.begin(), ::toupper);	
	string cmd = popword(line);
	
	if (cmd == "HELP") {
		string topic = popword(line);
		if (topic.size() == 0) {
			connection->write(\
												":==== imgcam help ===========================\n"
												":info:                   Print device info.");
		}
		else connection->write("ERR CMD HELP :TOPIC UNKOWN");
	}
	else if (cmd == "INFO") {
		connection->write(format("OK INFO %d %d %d :width height bpp", res.x, res.y, bpp));
	}
	else connection->write("ERR CMD :CMD UNKOWN");
}		

int ImgCamera::get_image_ptr(void **out) {
	*out = (void *) image;
	return 0;
}

int ImgCamera::thumbnail(uint8_t *out) {
	update(false);
	
	uint16_t *in = (uint16_t *) image;
	uint8_t *p = out;
	
	for(int y = 0; y < 32; y++)
		for(int x = 0 ; x < 32; x++)
			*p++ = in[res.x * y * (res.y / 32) + x * (res.x / 32)] >> (bpp - 8);
	
	return 0;
}

int ImgCamera::monitor(void *out, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) {
	if(x1 < 0)
		x1 = 0;
	if(y1 < 0)
		y1 = 0;
	if(scale < 1)
		scale = 1;
	if(x2 * scale > res.x)
		x2 = res.x / scale;
	if(y2 * scale > res.y)
		y2 = res.y / scale;
			
	uint16_t *data = (uint16_t *) image;
	uint16_t *p = (uint16_t *)out;
	
	for(int y = y1 * scale; y < y2 * scale; y += scale) {
		for(int x = x1 * scale; x < x2 * scale; x += scale) {
			*p++ = data[y * res.x + x];
		}
	}
	
	size = (p - (uint16_t *)out) * bpp/8;
	return size;							// Number of bytes
}

int ImgCamera::store(int fd) {
	update(true);
		
	size_t size = res.x * res.y * bpp/8;
	if ((size_t) write(fd, frame, size) != size)
		return -1;
	
	return 0;
}
