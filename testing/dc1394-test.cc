/*
 dc1394-test.cc -- test dc1394++.h routines
 Copyright (C) 2010 Tim van Werkhoven <t.i.m.vanwerkhoven@xs4all.nl>
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <vector>
#include "dc1394++.h"

using namespace std;

int main() {
	dc1394 dc1394obj;
	std::vector<dc1394::camera *> cameras = dc1394obj.find_cameras();

	if(!cameras.size())
		throw std::runtime_error("No IIDC cameras found.");
	if (cameras.size() != 1)
		fprintf(stderr, "dc1394-test.cc: Found multiple IIDC cameras, using the first one.");
	
	dc1394::camera *camera = cameras[0];
	camera->print_more_info();

//	camera->set_transmission(false);
//	camera->set_power(true);

//	camera->set_iso_speed((dc1394::iso_speed) dc1394::ISO_SPEED_800);
//	camera->set_framerate((dc1394::framerate) dc1394::FRAMERATE_30);
//	camera->set_video_mode((dc1394::video_mode) dc1394::VIDEO_MODE_640x480_MONO8);
//	camera->set_control_register(0x80c, 0x82040040);

//	camera->capture_setup(10);
//	camera->set_transmission(true);
	
	
	return 0;
}