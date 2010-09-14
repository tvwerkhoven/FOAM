/*
 dc1394++.h -- C++ abstraction of IEEE1394 Digital Camera library
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

#ifndef HAVE_DC1394PP_H
#define HAVE_DC1394PP_H

#include <dc1394/control.h>
#include <dc1394/register.h>
#include <dc1394/utils.h>
#include <vector>
#include <stdexcept>

class dc1394 {
	dc1394_t *handle;

	public:
	dc1394() {handle = dc1394_new(); if(!handle) throw exception("Unable to allocate dc1394 structure");}
	~dc1394() {dc1394_free(handle);}

	enum iso_speed {
		ISO_SPEED_100 = 0,
		ISO_SPEED_200,
		ISO_SPEED_400,
		ISO_SPEED_800,
		ISO_SPEED_1600,
		ISO_SPEED_3200,
	};

	enum framerate {
		FRAMERATE_1_875 = 32,
		FRAMERATE_3_75,
		FRAMERATE_7_5,
		FRAMERATE_15,
		FRAMERATE_30,
		FRAMERATE_60,
		FRAMERATE_120,
		FRAMERATE_240,
	};

	enum video_mode {
		VIDEO_MODE_160x120_YUV444 = 64,
		VIDEO_MODE_320x240_YUV422,
		VIDEO_MODE_640x480_YUV411,
		VIDEO_MODE_640x480_YUV422,
		VIDEO_MODE_640x480_RGB8,
		VIDEO_MODE_640x480_MONO8,
		VIDEO_MODE_640x480_MONO16,
		VIDEO_MODE_800x600_YUV422,
		VIDEO_MODE_800x600_RGB8,
		VIDEO_MODE_800x600_MONO8,
		VIDEO_MODE_1024x768_YUV422,
		VIDEO_MODE_1024x768_RGB8,
		VIDEO_MODE_1024x768_MONO8,
		VIDEO_MODE_800x600_MONO16,
		VIDEO_MODE_1024x768_MONO16,
		VIDEO_MODE_1280x960_YUV422,
		VIDEO_MODE_1280x960_RGB8,
		VIDEO_MODE_1280x960_MONO8,
		VIDEO_MODE_1600x1200_YUV422,
		VIDEO_MODE_1600x1200_RGB8,
		VIDEO_MODE_1600x1200_MONO8,
		VIDEO_MODE_1280x960_MONO16,
		VIDEO_MODE_1600x1200_MONO16,
		VIDEO_MODE_EXIF,
		VIDEO_MODE_FORMAT7_0,
		VIDEO_MODE_FORMAT7_1,
		VIDEO_MODE_FORMAT7_2,
		VIDEO_MODE_FORMAT7_3,
		VIDEO_MODE_FORMAT7_4,
		VIDEO_MODE_FORMAT7_5,
		VIDEO_MODE_FORMAT7_6,
		VIDEO_MODE_FORMAT7_7,
	};

	enum feature {
		FEATURE_BRIGHTNESS= 416,
		FEATURE_EXPOSURE,
		FEATURE_SHARPNESS,
		FEATURE_WHITE_BALANCE,
		FEATURE_HUE,
		FEATURE_SATURATION,
		FEATURE_GAMMA,
		FEATURE_SHUTTER,
		FEATURE_GAIN,
		FEATURE_IRIS,
		FEATURE_FOCUS,
		FEATURE_TEMPERATURE,
		FEATURE_TRIGGER,
		FEATURE_TRIGGER_DELAY,
		FEATURE_WHITE_SHADING,
		FEATURE_FRAME_RATE,
		/* 16 reserved features */
		FEATURE_ZOOM,
		FEATURE_PAN,
		FEATURE_TILT,
		FEATURE_OPTICAL_FILTER,
		/* 12 reserved features */
		FEATURE_CAPTURE_SIZE,
		FEATURE_CAPTURE_QUALITY
		/* 14 reserved features */
	};

	enum feature_mode {
		FEATURE_MODE_MANUAL= 736,
		FEATURE_MODE_AUTO,
		FEATURE_MODE_ONE_PUSH_AUTO
	};

	enum capture_policy {
		CAPTURE_POLICY_WAIT = 672,
		CAPTURE_POLICY_POLL,
	};

	class exception: public std::runtime_error {
		public:
		exception(const std::string reason): runtime_error(reason) {}
	};

	static void check(dc1394error_t err) { if(err) throw exception(dc1394_error_get_string(err)); }

	typedef dc1394video_frame_t frame;

	class camera {
		public:
		dc1394camera_t *handle;

		public:
		camera(dc1394camera_t *handle): handle(handle) {}
		~camera() { dc1394_camera_free(handle); }

		/* General system functions */
		void print_info(FILE *fd = stdout) { check(dc1394_camera_print_info(handle, fd)); }
		void set_broadcast(bool value = true) { check(dc1394_camera_set_broadcast(handle, (dc1394bool_t)value)); }
		void reset_bus() { check(dc1394_reset_bus(handle)); }

		/* Other functionalities */
		void reset() { check(dc1394_camera_reset(handle)); }
		void set_power(bool value = true) { check(dc1394_camera_set_power(handle, (dc1394switch_t)value)); }
		uint64_t get_guid() { return handle->guid; }
		std::string get_vendor() { return handle->vendor; }
		std::string get_model() { return handle->model; }

		/* Video functions */
		void set_framerate(framerate value) { check(dc1394_video_set_framerate(handle, (dc1394framerate_t)value)); }
		framerate get_framerate() { dc1394framerate_t value; check(dc1394_video_get_framerate(handle, &value)); return (framerate)value; }
		void set_video_mode(video_mode value) { check(dc1394_video_set_mode(handle, (dc1394video_mode_t)value)); }
		video_mode get_video_mode() { dc1394video_mode_t value; check(dc1394_video_get_mode(handle, &value)); return (video_mode)value; }
		void set_iso_speed(iso_speed value) { check(dc1394_video_set_iso_speed(handle, (dc1394speed_t)value)); }
		iso_speed get_iso_speed() { dc1394speed_t value; check(dc1394_video_get_iso_speed(handle, &value)); return (iso_speed)value; }
		void set_transmission(bool value = true) { check(dc1394_video_set_transmission(handle, (dc1394switch_t)value)); }
		bool get_transmission() { dc1394switch_t value; check(dc1394_video_get_transmission(handle, &value)); return value; }

		/* Features and registers */
		void set_feature(feature f, uint32_t value) { check(dc1394_feature_set_value(handle, (dc1394feature_t)f, value)); }
		uint32_t get_feature(feature f) { uint32_t value; check(dc1394_feature_get_value(handle, (dc1394feature_t)f, &value)); return value; }

		/* Register access */
		uint32_t get_register(uint64_t offset) { uint32_t value; check(dc1394_get_register(handle, offset, &value)); return value; }
		void set_register(uint64_t offset, uint32_t value) { check(dc1394_set_register(handle, offset, value)); }
		uint32_t get_control_register(uint64_t offset) { uint32_t value; check(dc1394_get_control_register(handle, offset, &value)); return value; }
		void set_control_register(uint64_t offset, uint32_t value) { check(dc1394_set_control_register(handle, offset, value)); }

		/* Capture functions */
		void capture_setup(uint32_t buffers) { check(dc1394_capture_setup(handle, buffers, DC1394_CAPTURE_FLAGS_DEFAULT)); }
		void capture_stop() { check(dc1394_capture_stop(handle)); }
		int get_capture_fileno() { return dc1394_capture_get_fileno(handle); }
		frame *capture_dequeue(capture_policy policy = CAPTURE_POLICY_WAIT) { frame *newframe = 0; check(dc1394_capture_dequeue(handle, (dc1394capture_policy_t)policy, &newframe)); return newframe; }
		void capture_enqueue(frame *oldframe) { check(dc1394_capture_enqueue(handle, oldframe)); }
	};

	std::vector<camera *> find_cameras() {
		std::vector<camera *> result;
		dc1394camera_list_t *cameralist;

		check(dc1394_camera_enumerate(handle, &cameralist));

		for(uint32_t i = 0; i < cameralist->num; i++)
			result.push_back(new camera(dc1394_camera_new(handle, cameralist->ids[i].guid)));

		dc1394_camera_free_list(cameralist);

		return result;
	}
};

#endif // HAVE_DC1394PP_H
