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
#include <stdint.h>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <math.h>

#include "types.h"

using namespace std;

class enumpair {
public:
	typedef int _eint;
	
	// Integer based mapping
	map<int, _eint> intmapfwd;
	map<_eint, int> intmapinv;
	
	void insert(_eint e, int val) {
		intmapfwd[val] = e;
		intmapinv[e] = val;
	}
	_eint getenum(int val) {
		return intmapfwd[val];
	}
	int getint(_eint e) {
		return intmapinv[e];
	}
	
	// String based mapping
	map<std::string, _eint> strmapfwd;
	map<_eint, std::string> strmapinv;
	
	void insert(_eint e, std::string val) {
		strmapfwd[val] = e;
		strmapinv[e] = val;
	}
	_eint getenum(std::string val) {
		return strmapfwd[val];
	}
	std::string getstr(_eint e) {
		return strmapinv[e];
	}
	
	// Double based mapping
	map<double, _eint> dblmapfwd;
	map<_eint, double> dblmapinv;
	
	void insert(_eint e, double val) {
		dblmapfwd[val] = e;
		dblmapinv[e] = val;
	}
	_eint getenum(double val) {
		return dblmapfwd[val];
	}
	double getdbl(_eint e) {
		return dblmapinv[e];
	}
};

class enumpair2 {
public:
	typedef int _eint;
	int curr, size;
	
	// String based mapping
	int *enumarr;
	std::string *strarr;
	
	void insert(_eint e, std::string val) {
		enumarr[curr] = e;
		strarr[curr] = val;
		curr++;
		// Realloc is space is needed
		if (curr >= size) {
			enumarr = (int *) realloc(enumarr, size + 16);
			strarr = (std::string *) realloc(strarr, size + 16);
			size += 16;
		}
	}
	_eint getenum(std::string val) {
		for (int i=0; i<curr; i++)
			if (strarr[i] == val)
				return enumarr[i];
		return -1;
	}
	std::string getstr(_eint e) {
		for (int i=0; i<curr; i++)
			if (enumarr[i] == e)
				return strarr[i];
		return "";
	}
	
	enumpair2() {
		curr = 0;
		size = 16;
		enumarr = (int *) malloc(size * sizeof(int));
		strarr = (std::string *) malloc(size * sizeof(std::string));
	}
	~enumpair2() {
		free(enumarr);
		free(strarr);
	}
};

class dc1394 {
	dc1394_t *handle;

	public:
	dc1394() { 
		handle = dc1394_new(); 
		if(!handle) throw std::runtime_error("Unable to allocate dc1394 structure");

		// Init enumpairs (cast to specific type for clarity)
		iso_speed_p.insert(ISO_SPEED_100, (int) 100);
		iso_speed_p.insert(ISO_SPEED_200, (int) 200);
		iso_speed_p.insert(ISO_SPEED_400, (int) 400);
		iso_speed_p.insert(ISO_SPEED_800, (int) 800);
		iso_speed_p.insert(ISO_SPEED_1600, (int) 1600);
		iso_speed_p.insert(ISO_SPEED_3200, (int) 3200);
		
		framerate_p.insert(FRAMERATE_1_875, (double) 1.875);
		framerate_p.insert(FRAMERATE_3_75, (double) 3.75);
		framerate_p.insert(FRAMERATE_7_5, (double) 7.5);
		framerate_p.insert(FRAMERATE_15, (double) 15.);
		framerate_p.insert(FRAMERATE_30, (double) 30.);
		framerate_p.insert(FRAMERATE_60, (double) 60.);
		framerate_p.insert(FRAMERATE_120, (double) 120.);
		framerate_p.insert(FRAMERATE_240, (double) 240.);

		video_mode_p.insert(VIDEO_MODE_160x120_YUV444, "VIDEO_MODE_160x120_YUV444");
		video_mode_p.insert(VIDEO_MODE_320x240_YUV422, "VIDEO_MODE_320x240_YUV422");
		video_mode_p.insert(VIDEO_MODE_640x480_YUV411, "VIDEO_MODE_640x480_YUV411");
		video_mode_p.insert(VIDEO_MODE_640x480_YUV422, "VIDEO_MODE_640x480_YUV422");
		video_mode_p.insert(VIDEO_MODE_640x480_RGB8, "VIDEO_MODE_640x480_RGB8");
		video_mode_p.insert(VIDEO_MODE_640x480_MONO8, "VIDEO_MODE_640x480_MONO8");
		video_mode_p.insert(VIDEO_MODE_640x480_MONO16, "VIDEO_MODE_640x480_MONO16");
		video_mode_p.insert(VIDEO_MODE_800x600_YUV422, "VIDEO_MODE_800x600_YUV422");
		video_mode_p.insert(VIDEO_MODE_800x600_RGB8, "VIDEO_MODE_800x600_RGB8");
		video_mode_p.insert(VIDEO_MODE_800x600_MONO8, "VIDEO_MODE_800x600_MONO8");
		video_mode_p.insert(VIDEO_MODE_1024x768_YUV422, "VIDEO_MODE_1024x768_YUV422");
		video_mode_p.insert(VIDEO_MODE_1024x768_RGB8, "VIDEO_MODE_1024x768_RGB8");
		video_mode_p.insert(VIDEO_MODE_1024x768_MONO8, "VIDEO_MODE_1024x768_MONO8");
		video_mode_p.insert(VIDEO_MODE_800x600_MONO16, "VIDEO_MODE_800x600_MONO16");
		video_mode_p.insert(VIDEO_MODE_1024x768_MONO16, "VIDEO_MODE_1024x768_MONO16");
		video_mode_p.insert(VIDEO_MODE_1280x960_YUV422, "VIDEO_MODE_1280x960_YUV422");
		video_mode_p.insert(VIDEO_MODE_1280x960_RGB8, "VIDEO_MODE_1280x960_RGB8");
		video_mode_p.insert(VIDEO_MODE_1280x960_MONO8, "VIDEO_MODE_1280x960_MONO8");
		video_mode_p.insert(VIDEO_MODE_1600x1200_YUV422, "VIDEO_MODE_1600x1200_YUV422");
		video_mode_p.insert(VIDEO_MODE_1600x1200_RGB8, "VIDEO_MODE_1600x1200_RGB8");
		video_mode_p.insert(VIDEO_MODE_1600x1200_MONO8, "VIDEO_MODE_1600x1200_MONO8");
		video_mode_p.insert(VIDEO_MODE_1280x960_MONO16, "VIDEO_MODE_1280x960_MONO16");
		video_mode_p.insert(VIDEO_MODE_1600x1200_MONO16, "VIDEO_MODE_1600x1200_MONO16");
		video_mode_p.insert(VIDEO_MODE_EXIF, "VIDEO_MODE_EXIF");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_0, "VIDEO_MODE_FORMAT7_0");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_1, "VIDEO_MODE_FORMAT7_1");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_2, "VIDEO_MODE_FORMAT7_2");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_3, "VIDEO_MODE_FORMAT7_3");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_4, "VIDEO_MODE_FORMAT7_4");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_5, "VIDEO_MODE_FORMAT7_5");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_6, "VIDEO_MODE_FORMAT7_6");
		video_mode_p.insert(VIDEO_MODE_FORMAT7_7, "VIDEO_MODE_FORMAT7_7");
		
		feature_p.insert(FEATURE_BRIGHTNESS, "FEATURE_BRIGHTNESS");
		feature_p.insert(FEATURE_EXPOSURE, "FEATURE_EXPOSURE");
		feature_p.insert(FEATURE_SHARPNESS, "FEATURE_SHARPNESS");
		feature_p.insert(FEATURE_WHITE_BALANCE, "FEATURE_WHITE_BALANCE");
		feature_p.insert(FEATURE_HUE, "FEATURE_HUE");
		feature_p.insert(FEATURE_SATURATION, "FEATURE_SATURATION");
		feature_p.insert(FEATURE_GAMMA, "FEATURE_GAMMA");
		feature_p.insert(FEATURE_SHUTTER, "FEATURE_SHUTTER");
		feature_p.insert(FEATURE_GAIN, "FEATURE_GAIN");
		feature_p.insert(FEATURE_IRIS, "FEATURE_IRIS");
		feature_p.insert(FEATURE_FOCUS, "FEATURE_FOCUS");
		feature_p.insert(FEATURE_TEMPERATURE, "FEATURE_TEMPERATURE");
		feature_p.insert(FEATURE_TRIGGER, "FEATURE_TRIGGER");
		feature_p.insert(FEATURE_TRIGGER_DELAY, "FEATURE_TRIGGER_DELAY");
		feature_p.insert(FEATURE_WHITE_SHADING, "FEATURE_WHITE_SHADING");
		feature_p.insert(FEATURE_FRAME_RATE, "FEATURE_FRAME_RATE");
		feature_p.insert(FEATURE_ZOOM, "FEATURE_ZOOM");
		feature_p.insert(FEATURE_PAN, "FEATURE_PAN");
		feature_p.insert(FEATURE_TILT, "FEATURE_TILT");
		feature_p.insert(FEATURE_OPTICAL_FILTER, "FEATURE_OPTICAL_FILTER");
		feature_p.insert(FEATURE_CAPTURE_SIZE, "FEATURE_CAPTURE_SIZE");
		feature_p.insert(FEATURE_CAPTURE_QUALITY, "FEATURE_CAPTURE_QUALITY");
	}
	~dc1394() {dc1394_free(handle);}

	enum iso_speed {
		ISO_SPEED_100 = 0,
		ISO_SPEED_200,
		ISO_SPEED_400,
		ISO_SPEED_800,
		ISO_SPEED_1600,
		ISO_SPEED_3200,
	};
	enumpair iso_speed_p;

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
	enumpair framerate_p;

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
	enumpair video_mode_p;

	// TODO: make struct of all supported modes for easy reference
//	typedef struct  {
//		video_mode mode;
//		framerate fps[DC1394_FRAMERATE_NUM];
//	};
//	video_modes_supp_t *video_mode_supp;

	
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
	enumpair feature_p;

	enum feature_mode {
		FEATURE_MODE_MANUAL= 736,
		FEATURE_MODE_AUTO,
		FEATURE_MODE_ONE_PUSH_AUTO
	};

	enum capture_policy {
		CAPTURE_POLICY_WAIT = 672,
		CAPTURE_POLICY_POLL,
	};

	static void check(dc1394error_t err) { if(err) throw std::runtime_error(dc1394_error_get_string(err)); }
	//static void check(dc1394error_t err) { if(err) exit(-1); }
	
	bool check_framerate(float fps) {
		// TODO: add check for current mode
		//if (log2(fps/1.875) < 0.0 || log2(fps/1.875) > 7.0 || (int) log2(fps/1.875) != log2(fps/1.875))
		if (fps == 1.875 || fps == 3.75 || fps == 7.5 || fps == 15.0 || fps == 30.0 || fps == 60.0 || fps == 120.0 || fps == 240.0) 
			return true;
		return false;
	}
	
	bool check_isospeed(int speed) {
		if (speed == 100 || speed == 200 || speed == 400 || speed == 800 || speed == 1600 || speed == 3200)
			return true;
		return false;
	}

	typedef dc1394video_frame_t frame;

	class camera {
	private:
		dc1394 *_dc1394;
		
	public:
		dc1394camera_t *handle;
		
	public:
		camera(dc1394camera_t *handle, dc1394 *parent): _dc1394(parent), handle(handle) {
			// TODO: query available video modes + framerates.
		}
		~camera() { dc1394_camera_free(handle); }

		/* General system functions */
		void print_info(FILE *fd = stdout) { check(dc1394_camera_print_info(handle, fd)); }
		void featprint(std::string featname, feature f, FILE *fd) {
			if (feature_present(f)) {
				if (feature_readable(f))
					fprintf(fd, "%-34s: readable; r: %d--%d; val: %d\n", featname.c_str(), get_feature_min(f), get_feature_max(f), get_feature(f));
				else
					fprintf(fd, "%-34s: unreadable\n", featname.c_str());
			}
			else
				fprintf(fd, "%-34s: not present.\n", featname.c_str());
		}
		void print_more_info(FILE *fd = stdout) { 
			check(dc1394_camera_print_info(handle, fd)); 
			uint32_t var1, var2, var3;
			dc1394feature_info_t featinfo;
			// Not implemented in libdc1394??? 
			//dc1394featureset_t features;
			//dc1394_get_camera_feature_set(handle, &features);
			//dc1394_print_feature_set(&features);
			
			fprintf(fd, "------ Features ----------------------------------\n");
			
			featprint("FEATURE_BRIGHTNESS", FEATURE_BRIGHTNESS, fd);
			featprint("FEATURE_EXPOSURE", FEATURE_EXPOSURE, fd);
			featprint("FEATURE_SHARPNESS", FEATURE_SHARPNESS, fd);
			
			if (feature_present(FEATURE_WHITE_BALANCE) && feature_readable(FEATURE_WHITE_BALANCE)) {
				dc1394_feature_whitebalance_get_value(handle, &var1, &var2);
				fprintf(fd, "%-34s: readable; u_b: %d, v_r: %d\n", "FEATURE_WHITE_BALANCE", var2, var1);
			}
			else
				fprintf(fd, "%-34s: not present.\n", "FEATURE_WHITE_BALANCE");
			
			featprint("FEATURE_HUE", FEATURE_HUE, fd);
			featprint("FEATURE_SATURATION", FEATURE_SATURATION, fd);
			featprint("FEATURE_GAMMA", FEATURE_GAMMA, fd);
			featprint("FEATURE_SHUTTER", FEATURE_SHUTTER, fd);
			featprint("FEATURE_GAIN", FEATURE_GAIN, fd);
			featprint("FEATURE_IRIS", FEATURE_IRIS, fd);
			featprint("FEATURE_FOCUS", FEATURE_FOCUS, fd);
			
			if (feature_present(FEATURE_TEMPERATURE) && feature_readable(FEATURE_TEMPERATURE)) {
				dc1394_feature_temperature_get_value(handle, &var1, &var2);
				fprintf(fd, "%-34s: readable; curr: %d, target: %d\n", "FEATURE_TEMPERATURE", var2, var1);
			}
			else
				fprintf(fd, "%-34s: not present.\n", "FEATURE_TEMPERATURE");
				

			featprint("FEATURE_TRIGGER", FEATURE_TRIGGER, fd);
			featprint("FEATURE_TRIGGER_DELAY", FEATURE_TRIGGER_DELAY, fd);
			
			if (feature_present(FEATURE_WHITE_SHADING) && feature_readable(FEATURE_WHITE_SHADING)) {
				dc1394_feature_whiteshading_get_value(handle, &var1, &var2, &var3);
				fprintf(fd, "%-34s: readable; r: %d, g: %d, b: %d\n", "FEATURE_WHITE_SHADING", var1, var2, var3);
			}
			else
				fprintf(fd, "%-34s: not present.\n", "FEATURE_WHITE_SHADING");
			
			featprint("FEATURE_ZOOM", FEATURE_ZOOM, fd);
			featprint("FEATURE_PAN", FEATURE_PAN, fd);
			featprint("FEATURE_TILT", FEATURE_TILT, fd);
			featprint("FEATURE_OPTICAL_FILTER", FEATURE_OPTICAL_FILTER, fd);
			featprint("FEATURE_CAPTURE_SIZE", FEATURE_CAPTURE_SIZE, fd);
			featprint("FEATURE_CAPTURE_QUALITY", FEATURE_CAPTURE_QUALITY, fd);
			
			fprintf(fd, "------ Video modes -------------------------------\n");
			dc1394video_modes_t modes;
			dc1394_video_get_supported_modes(handle, &modes);
			
			dc1394framerates_t  framerates;
			
			fprintf(fd, "Camera supports %d modes:\n", modes.num);
			int j;
			for (int modei=DC1394_VIDEO_MODE_MIN; modei < DC1394_VIDEO_MODE_MAX; modei++) {
				for (j=0; j < modes.num; j++) {
					if (modes.modes[j] == modei) {
						fprintf(fd, "%-34s: supported\n", _dc1394->video_mode_p.getstr(modei).c_str());
						dc1394_video_get_supported_framerates(handle, modes.modes[j], &framerates);
						
						fprintf(fd, "+-> Framerates: ");
						for (int fpsi=DC1394_FRAMERATE_MIN; fpsi < DC1394_FRAMERATE_MAX; fpsi++) {
							for (j=0; j < framerates.num; j++) {
								if (framerates.framerates[j] == fpsi)
									fprintf(fd, "%g ", _dc1394->framerate_p.getdbl(fpsi));
							}
						}
						fprintf(fd, "\n");
					}
				}
				if (j == modes.num)
					fprintf(fd, "%-34s: unsupported\n", _dc1394->video_mode_p.getstr(modei).c_str());
			}
						
		}
		void set_broadcast(bool value = true) { check(dc1394_camera_set_broadcast(handle, (dc1394bool_t)value)); }
		void reset_bus() { check(dc1394_reset_bus(handle)); }

		/* Other functionalities */
		void reset() { check(dc1394_camera_reset(handle)); }
		void set_power(bool value = true) { check(dc1394_camera_set_power(handle, (dc1394switch_t)value)); }
		uint64_t get_guid() { return handle->guid; }
		uint64_t get_uid() { return handle->unit_spec_ID; }
		std::string get_vendor() { return handle->vendor; }
		std::string get_model() { return handle->model; }

		/* Video functions */
		void set_framerate(framerate value) { check(dc1394_video_set_framerate(handle, (dc1394framerate_t)value)); }
		framerate get_framerate() { dc1394framerate_t value; check(dc1394_video_get_framerate(handle, &value)); return (framerate)value; }
		void set_framerate_f(double fps) { 
			if (!_dc1394->check_framerate(fps))
				return;
			
			check(dc1394_video_set_framerate(handle, (dc1394framerate_t) _dc1394->framerate_p.getenum(fps))); 
		}
		double get_framerate_f() { 
			float fps; 
			dc1394_framerate_as_float((dc1394framerate_t) get_framerate(), &fps);
			return fps;
		}
		
		void set_video_mode(video_mode value) { check(dc1394_video_set_mode(handle, (dc1394video_mode_t)value)); }
		video_mode get_video_mode() { dc1394video_mode_t value; check(dc1394_video_get_mode(handle, &value)); return (video_mode)value; }
		void set_iso_speed(iso_speed value) { check(dc1394_video_set_iso_speed(handle, (dc1394speed_t)value)); }
		iso_speed get_iso_speed() { dc1394speed_t value; check(dc1394_video_get_iso_speed(handle, &value)); return (iso_speed)value; }
		void set_transmission(bool value = true) { check(dc1394_video_set_transmission(handle, (dc1394switch_t)value)); }
		bool get_transmission() { dc1394switch_t value; check(dc1394_video_get_transmission(handle, &value)); return value; }

		/* Features and registers */
		void set_feature(feature f, uint32_t value) { check(dc1394_feature_set_value(handle, (dc1394feature_t)f, value)); }
		uint32_t get_feature(feature f) { uint32_t value; check(dc1394_feature_get_value(handle, (dc1394feature_t)f, &value)); return value; }
		bool feature_present(feature f) { dc1394bool_t  value; check(dc1394_feature_is_present(handle, (dc1394feature_t)f, &value)); return value; }
		bool feature_readable(feature f) { dc1394bool_t  value; check(dc1394_feature_is_readable(handle, (dc1394feature_t)f, &value)); return value; } 
		uint32_t get_feature_min(feature f) { uint32_t min, max; check(dc1394_feature_get_boundaries(handle, (dc1394feature_t)f, &min, &max)); return min; }
		uint32_t get_feature_max(feature f) { uint32_t min, max; check(dc1394_feature_get_boundaries(handle, (dc1394feature_t)f, &min, &max)); return max; }
		
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
			result.push_back(new camera(dc1394_camera_new(handle, cameralist->ids[i].guid), this));

		dc1394_camera_free_list(cameralist);

		return result;
	}
};

#endif // HAVE_DC1394PP_H
