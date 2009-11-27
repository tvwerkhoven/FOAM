/*
    itifgccd.cc -- Camera Control Daemon for the Dutch Open Telescope
    Copyright (C) 2006  Guus Sliepen <guus@sliepen.eu.org>

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

#include "cam.h"
#include "pthread++.h"

using namespace std;

class DummyCamera: public Camera {
	pthread::mutex mutex;
	int width;
	int height;
	int depth;
	double interval;
	double exposure;
	double noise;
	uint16_t *frame;
	Camera::mode mode;
	int offset;

	void update(bool blocking) {
		io->msg(IO_DEB2, "DummyCamera::update()");
		if(blocking)
			usleep(interval * 1000000);

		uint16_t *p = frame;
		int mul = (1 << depth) - 1;
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < height; x++) {
				double value = drand48() * noise + (sin(M_PI * x / width) + 1 + sin((y + offset) * 100));
				value *= exposure;
				if(value < 0)
					value = 0;
				if(value > 1)
					value = 1;
				*p++ = (uint16_t)(value * mul) & mul;
			}
		}
		offset++;
		if(offset > 1000)
			offset = 0;
	}

	public:
	DummyCamera(config &config) {
		io->msg(IO_DEB2, "DummyCamera::DummyCamera(config &config)");
		width = config.getint("width", 512);
		height = config.getint("height", 512);
		depth = config.getint("depth", 16);
		noise = config.getdouble("noise", 0.001);
		interval = 0.25;
		exposure = 0.3;
		bpp = 16;
		dtype = DATA_UINT16;
		mode = Camera::OFF;
		frame = (uint16_t *)malloc(width * height * 2);
		if(!frame)
			throw exception("Could not allocate memory for framebuffer");
		io->msg(IO_INFO, "DummyCamera init success, got %dx%dx%d frame, noise=%g, intv=%g, exp=%g.", 
						width, height, depth, noise, interval, exposure);
	}

	~DummyCamera() {
		io->msg(IO_DEB2, "DummyCamera::~DummyCamera()");
	}

	double get_interval() {
		return interval;
	}

	double get_exposure() {
		return exposure;
	}

	int get_width() {
		return width;
	}

	int get_height() {
		return height;
	}

	int get_depth() {
		return depth;
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
				 *p++ = in[width * y * (height / 32) + x * (width / 32)] >> (depth - 8);

		return true;
	}
	
	bool get_image(void **out) {
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
		if(x2 * scale > width)
			x2 = width / scale;
		if(y2 * scale > height)
			y2 = height / scale;

		update(true);

		pthread::mutexholder h(&mutex);

		uint16_t *data = frame;
		uint16_t *p = (uint16_t *)out;
		
		for(int y = y1 * scale; y < y2 * scale; y += scale) {
			for(int x = x1 * scale; x < x2 * scale; x += scale) {
				*p++ = data[y * width + x];
			}
		}

		size = (p - (uint16_t *)out) * 2;
		return true;
	}

	void init_capture() {
		//update(true);
	}	
		
	bool capture(int fd) {
		update(true);

		pthread::mutexholder h(&mutex);

		size_t size = width * height * 2;
		if(write(fd, frame, size != size))
			return false;

		return true;
	}
};

Camera *Camera::create(config &config) {
	io->msg(IO_DEB2, "Camera::create(config &config)");
	return new DummyCamera(config);
}
