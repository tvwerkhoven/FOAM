/*
    dc1394.cc -- IEEE1394 Digital Camera handler for the Dutch Open Telescope
    Copyright (C) 2006-2007  Guus Sliepen <G.Sliepen@astro.uu.nl>

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

#include "camera.h"

#include "config.h"
#include "dc1394++.h"
#include "pthread++.h"

dc1394 dc1394;

using namespace std;

dc1394::camera *camera;
pthread::thread thread;

double camera_get_exposure() {
	pthread::mutexholder h(&mutex);
	return camera->get_feature(dc1394::FEATURE_EXPOSURE) / 30.0 / 512;
}

double camera_get_interval() {
	pthread::mutexholder h(&mutex);
	return 1.0 / 1.875 / (1 << (camera->get_framerate() - 32));
}

double camera_get_gain() {
	pthread::mutexholder h(&mutex);
	return camera->get_feature(dc1394::FEATURE_GAIN);
}

double camera_get_offset() {
	pthread::mutexholder h(&mutex);
	return camera->get_feature(dc1394::FEATURE_BRIGHTNESS) - 256;
}

void camera_set_exposure(double value) {
	pthread::mutexholder h(&mutex);
	camera->set_feature(dc1394::FEATURE_EXPOSURE, max((uint32_t)(value * 30 * 512), (uint32_t)511));
	exposure = camera_get_exposure();
}

void camera_set_interval(double value) {
}

void camera_set_gain(double value) {
	pthread::mutexholder h(&mutex);
	camera->set_feature(dc1394::FEATURE_GAIN, (uint32_t)value);
	gain = camera_get_gain();
}

void camera_set_offset(double value) {
	pthread::mutexholder h(&mutex);
	camera->set_feature(dc1394::FEATURE_BRIGHTNESS, (uint32_t)value + 256);
	offset = camera_get_offset();
}

void handler() {
	pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(1, &cpuset);
	pthread::setaffinity(&cpuset);

	while(true) {
		auto frame = camera->capture_dequeue(dc1394::CAPTURE_POLICY_WAIT);
		if(!frame) {
			timeouts++;
			usleep(50000);
			continue;
		}

		auto oldframe = (dc1394::frame *)queue(frame, frame->image);
		if(oldframe)
			camera->capture_enqueue(oldframe);
	}
}

void camera_init() {
	auto cameras = dc1394.find_cameras();
	if(!cameras.size())
		throw runtime_error("No IIDC cameras found.");
	camera = cameras[0];
	camera->set_transmission(false);
	camera->set_power();
	camera->set_iso_speed(dc1394::ISO_SPEED_400);
	camera->set_framerate(dc1394::FRAMERATE_30);
	camera->set_video_mode(dc1394::VIDEO_MODE_640x480_MONO8);
	camera->set_control_register(0x80c, 0x82040040);
	camera->capture_setup(nframes + 10);
	camera->set_transmission();

	width = 640;
	height = 480;
	depth = 8;
	exposure = camera_get_exposure();
	interval = camera_get_interval();
	gain = camera_get_gain();
	offset = camera_get_offset();

	thread.create(sigc::ptr_fun(handler));
}

void camera_exit() {
	thread.cancel();
	thread.join();
	camera->set_transmission(false);
	camera->capture_stop();
	camera->set_power(false);
}
