/*
    imgcam.cc -- Dummy 'camera' with static images as source
    Copyright (C) 2009 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>

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
#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "imgio.h"
#include "cam.h"
#include "pthread++.h"

#define IMGCAM_TYPE "imgcam"

using namespace std;

class ImgCamera: public Camera {
	pthread::mutex mutex;
	
	double interval;
	double exposure;
	double noise;
		
	uint16_t *frame;
	
	Imgio *img;

	Camera::mode mode;
	int offset;

	void update(bool blocking) {
		// Copy image from img to frame and add some noise etc
		io->msg(IO_DEB2, "ImgCamera::update()");
		if(blocking)
			usleep((int) interval * 1000000);

		uint16_t *p = frame;

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
	
public:
	ImgCamera(config &config) {
		io->msg(IO_DEB2, "ImgCamera::ImgCamera(config &config)");
		
		string type = config.getstring("type");
		if (type != IMGCAM_TYPE) throw exception("Type should be " IMGCAM_TYPE " for this class.");
		
		string file = config.getstring("imagefile");
		if (file.length()) {
			if (file[0] != '/') file = FOAM_DATADIR "/" + file;
		}
		else {
			throw exception("'imagefile' not set in configuration file!");
		}
		io->msg(IO_DEB2, "ImgCamera::ImgCamera(): imagefile = %s", file.c_str());
		noise = config.getdouble("noise", 10.0);
		interval = config.getdouble("interval", 0.25);
		exposure = config.getdouble("exposure", 1.0);
		
		mode = Camera::OFF;
		
		img = new Imgio(file, Imgio::FITS);
		img->loadImg();
		
		res.x = img->getWidth();
		res.y = img->getHeight();
		bpp = 16;
		dtype = DATA_UINT16;
		
		frame = (uint16_t *) malloc(res.x * res.y * 2);
		
		if (!frame)
			throw exception("Could not allocate memory for framebuffer");
		
		update(true);

		io->msg(IO_INFO, "ImgCamera init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
						res.x, res.y, bpp, noise, interval, exposure);
		io->msg(IO_INFO, "Range = %d--%d, sum=%lld", img->range[0], img->range[1], img->sum);
	}

	~ImgCamera() {
		io->msg(IO_DEB2, "ImgCamera::~ImgCamera()");
		delete img;
	}

	double get_interval() {
		return interval;
	}

	double get_exposure() {
		return exposure;
	}

	int get_width() {
		return res.x;
	}

	int get_height() {
		return res.y;
	}

	int get_depth() {
		return bpp;
	}

	void set_mode(Camera::mode newmode) {
		mode = newmode;
	}

	Camera::mode get_mode() {
		return mode;
	}

	void set_interval(double value) {
		interval = value;
	}

	void set_exposure(double value) {
		exposure = value;
	}

	bool thumbnail(uint8_t *out) {
		update(false);

		pthread::mutexholder h(&mutex);

		uint16_t *in = frame;
		uint8_t *p = out;
		for(int y = 0; y < 32; y++)
			for(int x = 0 ; x < 32; x++)
				 *p++ = in[res.x * y * (res.y / 32) + x * (res.x / 32)] >> (bpp - 8);

		return true;
	}
	
	bool get_image(void **out) {
		update(true);
		*out = (void *) frame;
		return true;
	}

	bool monitor(void *out, size_t &size, int &x1, int &y1, int &x2, int &y2, int &scale) {
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

		update(true);

		pthread::mutexholder h(&mutex);

		uint16_t *data = frame;
		uint16_t *p = (uint16_t *)out;
		
		for(int y = y1 * scale; y < y2 * scale; y += scale) {
			for(int x = x1 * scale; x < x2 * scale; x += scale) {
				*p++ = data[y * res.x + x];
			}
		}

		size = (p - (uint16_t *)out) * bpp/8;
		return true;
	}

	void init_capture() {
		//update(true);
	}	
		
	bool capture(int fd) {
		update(true);

		pthread::mutexholder h(&mutex);

		size_t size = res.x * res.y * bpp/8;
		if(write(fd, frame, size != size))
			return false;

		return true;
	}
};

Camera *Camera::create(config &config) {
	io->msg(IO_DEB2, "Camera::create(config &config)");
	return new ImgCamera(config);
}
