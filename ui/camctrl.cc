#include <iostream>

#include "camera.h"
#include "format.h"

#include <arpa/inet.h>

using namespace std;

Camera::Camera(const string h, const string p, const string n): 
	monitorprotocol(host, port, "") 
{
		
	DeviceCtrl::DeviceCtrl(h, p, n);
	ok = false;
	state = UNDEFINED;
	errormsg = "Not connected";
	
	exposure = 0;
	interval = 0;
	gain = 0;
	offset = 0;
	width = 0;
	height = 0;
	depth = 0;
	mode = OFF;
	r = 1;
	g = 1;
	b = 1;

	monitor.x1 = 0;
	monitor.y1 = 0;
	monitor.x2 = 0;
	monitor.y2 = 0;
	monitor.scale = 1;
	monitor.image = 0;
	monitor.size = 0;
	monitor.histogram = 0;

//	protocol.slot_message = sigc::mem_fun(this, &Camera::on_message);
//	protocol.slot_connected = sigc::mem_fun(this, &Camera::on_connected);
//	protocol.connect();
	monitorprotocol.slot_message = sigc::mem_fun(this, &Camera::on_monitor_message);
	monitorprotocol.connect();
}

Camera::~Camera() {
	set_off();
}

void Camera::on_connect(bool connected) {
	DeviceCtrl::on_connect(connected);
	
	printf("Camera::on_connect()");
	if(!connected) {
		ok = false;
		errormsg = "Not connected";
		signal_update();
		return;
	}

	protocol.write("get exposure");
	protocol.write("get interval");
	protocol.write("get gain");
	protocol.write("get offset");
	protocol.write("get width");
	protocol.write("get height");
	protocol.write("get depth");
	protocol.write("get mode");
	protocol.write("get filename");
	protocol.write("get state");
}

void Camera::on_message(string line) {
	DeviceCtrl::on_message(line);
	
	if(!ok) {
		state = ERROR;
		return;
	}

	string what = popword(line);

	if(what == "exposure")
		exposure = popdouble(line);
	else if(what == "interval")
		interval = popdouble(line);
	else if(what == "gain")
		gain = popdouble(line);
	else if(what == "offset")
		offset = popdouble(line);
	else if(what == "width")
		width = popint32(line);
	else if(what == "height")
		height = popint32(line);
	else if(what == "depth")
		depth = popint32(line);
	else if(what == "filename")
		filename = popword(line);
	else if(what == "state") {
		string statename = popword(line);
		if(statename == "ready")
			state = READY;
		else if(statename == "waiting")
			state = WAITING;
		else if(statename == "burst")
			state = BURST;
		else if(statename == "error") {
			state = ERROR;
			ok = false;
		} else {
			state = UNDEFINED;
			ok = false;
			errormsg = "Unexpected state '" + statename + "'";
		}
	} else if(what == "mode") {
		string modename = popword(line);
		if(modename == "master")
			mode = MASTER;
		else if(modename == "slave")
			mode = SLAVE;
		else
			mode = OFF;
	} else if(what == "thumbnail") {
		protocol.read(thumbnail, sizeof thumbnail);
		signal_thumbnail();
		return;
	} else if(what == "fits") {
	} else {
		//ok = false;
		//errormsg = "Unexpected response '" + what + "'";
	}

	signal_update();
}

void Camera::on_monitor_message(string line) {
	if(popword(line) != "OK")
		return;
	
	if(popword(line) != "image")
		return;

	size_t size = popsize(line);
	int histosize = (1 << depth) * sizeof *monitor.histogram;
	int x1 = popint(line);
	int y1 = popint(line);
	int x2 = popint(line);
	int y2 = popint(line);
	int scale = popint(line);
	double dx = 0.0/0.0;
	double dy = 0.0/0.0;
	double cx = 0.0/0.0;
	double cy = 0.0/0.0;
	double cr = 0.0/0.0;
	bool do_histogram = false;

	string extra;

	while(!(extra = popword(line)).empty()) {
		if(extra == "histogram") {
			do_histogram = true;
		} else if(extra == "tiptilt") {
			dx = popdouble(line);
			dy = popdouble(line);
		} else if(extra == "com") {
			cx = popdouble(line);
			cy = popdouble(line);
			cr = popdouble(line);
		}
	}

	{
		pthread::mutexholder h(&monitor.mutex);
		if(size > monitor.size)
			monitor.image = (uint16_t *)realloc(monitor.image, size);
		monitor.size = size;
		monitor.x1 = x1;
		monitor.y1 = y1;
		monitor.x2 = x2;
		monitor.y2 = y2;
		monitor.scale = scale;
		monitor.dx = dx;
		monitor.dy = dy;
		monitor.cx = cx;
		monitor.cy = cy;
		monitor.cr = cr;
		monitor.depth = depth;

		if(do_histogram)
			monitor.histogram = (uint32_t *)realloc(monitor.histogram, histosize);
	}

	monitorprotocol.read(monitor.image, monitor.size);

	if(do_histogram)
		monitorprotocol.read(monitor.histogram, histosize);

	signal_monitor();
}

void Camera::get_thumbnail() {
	protocol.write("thumbnail");
}

double Camera::get_exposure() const {
	return exposure;
}

double Camera::get_interval() const {
	return interval;
}

double Camera::get_gain() const {
	return gain;
}

double Camera::get_offset() const {
	return offset;
}

int32_t Camera::get_width() const {
	return width;
}

int32_t Camera::get_height() const {
	return height;
}

int32_t Camera::get_depth() const {
	return depth;
}

std::string Camera::get_filename() const {
	return filename;
}

bool Camera::is_master() const {
	return mode == MASTER;
}

bool Camera::is_slave() const {
	return mode == SLAVE;
}

bool Camera::is_off() const {
	return mode == OFF;
}

bool Camera::is_enabled() const {
	return enabled;
}

void Camera::set_exposure(double value) {
	protocol.write(format("set exposure %lf", value));
}

void Camera::set_interval(double value) {
	protocol.write(format("set interval %lf", value));
}

void Camera::set_gain(double value) {
	protocol.write(format("set gain %lf", value));
}

void Camera::set_offset(double value) {
	protocol.write(format("set offset %lf", value));
}

void Camera::set_master() {
	protocol.write("set mode master");
}

void Camera::set_slave() {
	protocol.write("set mode slave");
}

void Camera::set_off() {
	protocol.write("set mode off");
}

void Camera::set_enabled(bool value) {
	enabled = value;
}

void Camera::set_filename(const string &filename) {
	protocol.write("set filename :" + filename);
}

void Camera::set_fits(const string &fits) {
	protocol.write("set fits " + fits);
}

void Camera::darkburst(int32_t count) {
	string command = format("dark %d", count);
	state = UNDEFINED;
	protocol.write(command);
}

void Camera::flatburst(int32_t count) {
	string command = format("flat %d", count);
	state = UNDEFINED;
	protocol.write(command);
}

void Camera::burst(int32_t count, int32_t fsel) {
	string command = format("burst %d", count);
	if(fsel > 1)
		command += format(" select %d", fsel);
	state = UNDEFINED;
	protocol.write(command);
}

enum Camera::state Camera::get_state() const {
	return state;
}

bool Camera::wait_for_state(enum state desiredstate, bool condition) {
	while((ok && state != ERROR) && (state == UNDEFINED || (state == desiredstate) != condition))
		usleep(100000);

	return (state == desiredstate) == condition;
}

//bool Camera::is_ok() const {
//	return ok;
//}
//
//string Camera::get_errormsg() const {
//	return errormsg;
//}

void Camera::grab(int x1, int y1, int x2, int y2, int scale, bool darkflat, int fsel) {
	string command = format("grab %d %d %d %d %d histogram", x1, y1, x2, y2, scale);
	if(darkflat)
		command += " darkflat";
	if(fsel)
		command += format(" select %d", fsel);
	monitorprotocol.write(command);
}
