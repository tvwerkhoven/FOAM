/*
 andor.cc -- Andor iXON camera modules
 Copyright (C) 2011-2012 Tim van Werkhoven <werkhoven@strw.leidenuniv.nl>
 
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

#include <string>
#include <vector>
#include <stdexcept>

#ifdef __GNUC__
#  if(__GNUC__ > 3 || __GNUC__ ==3)
#	define _GNUC3_
#  endif
#endif
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <atmcdLXd.h> // For Andor camera

#include "pthread++.h"
#include "config.h"
#include "path++.h"
#include "io.h"

#include "foam.h"
#include "foamctrl.h"
#include "camera.h"
#include "andor.h"

using namespace std;

AndorCam::AndorCam(Io &io, foamctrl *const ptc, const string name, const string port, Path const &conffile, const bool online):
Camera(io, ptc, name, andor_type, port, conffile, online),
andordir("/")
{
	io.msg(IO_DEB2, "AndorCam::AndorCam()");
	int ret=0;
	
	// Register network commands here
	add_cmd("set cooling");
	add_cmd("get cooling");
	add_cmd("get temperature");
	add_cmd("set frametransfer");
	add_cmd("get frametransfer");
	
	// Set static parameters
	emgain_range[0] = emgain_range[1] = 0;
	
	// Get configuration parameters
	int cooltemp = cfg.getint("cooltemp", 20);
	andordir = cfg.getstring("andor_cfgdir", "/usr/local/etc/andor");
	
	int hsspeed = cfg.getint("hsspeed", 0);
	int vsspeed = cfg.getint("vsspeed", 0);
	int vsamp = cfg.getint("vsamp", 4);
	
	int pagain = cfg.getint("pa_gain", 2);
  
	frametransfer = cfg.getint("frametransfer", 0);
	
	// EM gain settings
	int emgain_m = cfg.getint("emccdgain_mode", 0);
	int emgain_init = cfg.getint("emccdgain_init", 0);
	
	// Init error codes
	init_errors();	

	// Initialize default configuration
	ret = initialize();
	if (ret != DRV_SUCCESS) {
		ShutDown();
		io.msg(IO_ERR, "AndorCam::AndorCam() Could not initialize andor camera, error: %d, %s", ret, error_desc[ret].c_str());
		throw std::runtime_error(format("Could not initialize andor camera! error: %d, %s", ret, error_desc[ret].c_str()));
	}
	io.msg(IO_INFO, "AndorCam::AndorCam() init complete!");
	
	// Get camera capabilities
	cam_get_capabilities();

	// Get cooling temperature range
	io.msg(IO_DEB1, "AndorCam::AndorCam() setting cooling...");
	cam_get_coolrange();
	cam_set_cooltarget(cooltemp);
	
	// Set CCD readout speed-related settings
	io.msg(IO_DEB1, "AndorCam::AndorCam() setting shift speed...");
	cam_set_shift_speed(hsspeed, vsspeed, vsamp);
	
	// Set pre-amp gain (should be left alone, probably)
	io.msg(IO_DEB1, "AndorCam::AndorCam() setting gain...");
	ret = SetPreAmpGain(pagain);				// 0: 1x, 1: 2.2x, 2: 4.6x. Andor recommends 4.6x (iXonEM+ Hardware guide 3.3.1)
	if (ret != DRV_SUCCESS) 
		io.msg(IO_ERR, "AndorCam::AndorCam() SetPreAmpGain error: %d, %s", ret, error_desc[ret].c_str());
  
	// Set frame transfer
	cam_set_frametranfer(frametransfer);

	// Set gain mode (automatically queries the gain range as well)
	cam_set_gain_mode(emgain_m);
	
	// Set gain to initial value (probably zero or so)
	cam_set_gain(emgain_init);
	
	io.msg(IO_DEB1, "AndorCam::AndorCam() setting exposure...");
	cam_set_exposure(exposure);
	cam_set_interval(interval);
		
	// Setup image buffers
	img_buffer.resize(nframes);
	for (size_t i=0; i<nframes; i++)
		img_buffer.at(i) = new unsigned short[res.x * res.y];
	
	// Set filename prefix for saved frames
	set_filename("andor-");
	
	io.msg(IO_INFO, "AndorCam init success, got %dx%dx%d frame, intv=%g, exp=%g.", 
				 res.x, res.y, depth, interval, exposure);
	
	// Start camera thread
	cam_set_mode(Camera::WAITING);
	cam_thr.create(sigc::mem_fun(*this, &AndorCam::cam_handler));
	// Give thread time to initialize
	//! @todo Solve synchronisation with a mutex instead!
	sleep(1);
}

AndorCam::~AndorCam() {
	io.msg(IO_DEB2, "AndorCam::~AndorCam()");
	
	// Acquisition off
	io.msg(IO_DEB2, "AndorCam::~AndorCam() joining cam_handler() thread");
	cam_set_mode(Camera::OFF);
	// Stop capture thread
	cam_thr.cancel();
	cam_thr.join();
	sleep(1);
	
	// Abort acquisition and close shutter
	AbortAcquisition();
	cam_set_shutter(SHUTTER_CLOSED);
	sleep(1);

	// Disable cooler, warm up CCD
	cam_set_cooltarget(15);
	cam_set_cooler(false);
	int temp = cam_get_cooltemp();
	while (temp < 5) {
		temp = cam_get_cooltemp();
		io.msg(IO_INFO, "AndorCam::~AndorCam() waiting for camera to warm up (temp == %d < 5).", temp);
		net_broadcast(format("ok shutdown :waiting for camera to warm up (temp == %d < 5).", temp));
		sleep(10);
	}
	io.msg(IO_INFO, "AndorCam::~AndorCam() camera warmed up (temp == %d >= 5).", temp);
	
	io.msg(IO_INFO, "AndorCam::~AndorCam() Shutting down");
	ShutDown();
	
	io.msg(IO_INFO, "AndorCam::~AndorCam() Releasing memory (%zu items)", img_buffer.size());
	// Delete frames in buffer if necessary
	for (size_t i=0; i < img_buffer.size(); i++)
		delete[] img_buffer.at(i);
	
	io.msg(IO_INFO, "AndorCam::~AndorCam() done.");
}

void AndorCam::init_errors() {
	error_desc[20001] = "DRV_ERROR_CODES";
	error_desc[20002] = "DRV_SUCCESS";
	error_desc[20003] = "DRV_VXDNOTINSTALLED";
	error_desc[20004] = "DRV_ERROR_SCAN";
	error_desc[20005] = "DRV_ERROR_CHECK_SUM";
	error_desc[20006] = "DRV_ERROR_FILELOAD";
	error_desc[20007] = "DRV_UNKNOWN_FUNCTION";
	error_desc[20008] = "DRV_ERROR_VXD_INIT";
	error_desc[20009] = "DRV_ERROR_ADDRESS";
	error_desc[20010] = "DRV_ERROR_PAGELOCK";
	error_desc[20011] = "DRV_ERROR_PAGEUNLOCK";
	error_desc[20012] = "DRV_ERROR_BOARDTEST";
	error_desc[20013] = "DRV_ERROR_ACK";
	error_desc[20014] = "DRV_ERROR_UP_FIFO";
	error_desc[20015] = "DRV_ERROR_PATTERN";
	
	error_desc[20017] = "DRV_ACQUISITION_ERRORS";
	error_desc[20018] = "DRV_ACQ_BUFFER";
	error_desc[20019] = "DRV_ACQ_DOWNFIFO_FULL";
	error_desc[20020] = "DRV_PROC_UNKONWN_INSTRUCTION";
	error_desc[20021] = "DRV_ILLEGAL_OP_CODE";
	error_desc[20022] = "DRV_KINETIC_TIME_NOT_MET";
	error_desc[20023] = "DRV_ACCUM_TIME_NOT_MET";
	error_desc[20024] = "DRV_NO_NEW_DATA";
	error_desc[20025] = "KERN_MEM_ERROR";
	error_desc[20026] = "DRV_SPOOLERROR";
	error_desc[20027] = "DRV_SPOOLSETUPERROR";
	error_desc[20028] = "DRV_FILESIZELIMITERROR";
	error_desc[20029] = "DRV_ERROR_FILESAVE";
	
	error_desc[20033] = "DRV_TEMPERATURE_CODES";
	error_desc[20034] = "DRV_TEMPERATURE_OFF";
	error_desc[20035] = "DRV_TEMPERATURE_NOT_STABILIZED";
	error_desc[20036] = "DRV_TEMPERATURE_STABILIZED";
	error_desc[20037] = "DRV_TEMPERATURE_NOT_REACHED";
	error_desc[20038] = "DRV_TEMPERATURE_OUT_RANGE";
	error_desc[20039] = "DRV_TEMPERATURE_NOT_SUPPORTED";
	error_desc[20040] = "DRV_TEMPERATURE_DRIFT";
	
	error_desc[20033] = "DRV_TEMP_CODES";
	error_desc[20034] = "DRV_TEMP_OFF";
	error_desc[20035] = "DRV_TEMP_NOT_STABILIZED";
	error_desc[20036] = "DRV_TEMP_STABILIZED";
	error_desc[20037] = "DRV_TEMP_NOT_REACHED";
	error_desc[20038] = "DRV_TEMP_OUT_RANGE";
	error_desc[20039] = "DRV_TEMP_NOT_SUPPORTED";
	error_desc[20040] = "DRV_TEMP_DRIFT";
	
	error_desc[20049] = "DRV_GENERAL_ERRORS";
	error_desc[20050] = "DRV_INVALID_AUX";
	error_desc[20051] = "DRV_COF_NOTLOADED";
	error_desc[20052] = "DRV_FPGAPROG";
	error_desc[20053] = "DRV_FLEXERROR";
	error_desc[20054] = "DRV_GPIBERROR";
	error_desc[20055] = "DRV_EEPROMVERSIONERROR";
	
	error_desc[20064] = "DRV_DATATYPE";
	error_desc[20065] = "DRV_DRIVER_ERRORS";
	error_desc[20066] = "DRV_P1INVALID";
	error_desc[20067] = "DRV_P2INVALID";
	error_desc[20068] = "DRV_P3INVALID";
	error_desc[20069] = "DRV_P4INVALID";
	error_desc[20070] = "DRV_INIERROR";
	error_desc[20071] = "DRV_COFERROR";
	error_desc[20072] = "DRV_ACQUIRING";
	error_desc[20073] = "DRV_IDLE";
	error_desc[20074] = "DRV_TEMPCYCLE";
	error_desc[20075] = "DRV_NOT_INITIALIZED";
	error_desc[20076] = "DRV_P5INVALID";
	error_desc[20077] = "DRV_P6INVALID";
	error_desc[20078] = "DRV_INVALID_MODE";
	error_desc[20079] = "DRV_INVALID_FILTER";
	
	error_desc[20080] = "DRV_I2CERRORS";
	error_desc[20081] = "DRV_I2CDEVNOTFOUND";
	error_desc[20082] = "DRV_I2CTIMEOUT";
	error_desc[20083] = "DRV_P7INVALID";
	error_desc[20084] = "DRV_P8INVALID";
	error_desc[20085] = "DRV_P9INVALID";
	error_desc[20086] = "DRV_P10INVALID";
	error_desc[20087] = "DRV_P11INVALID";
	
	error_desc[20089] = "DRV_USBERROR";
	error_desc[20090] = "DRV_IOCERROR";
	error_desc[20091] = "DRV_VRMVERSIONERROR";
	error_desc[20093] = "DRV_USB_INTERRUPT_ENDPOINT_ERROR";
	error_desc[20094] = "DRV_RANDOM_TRACK_ERROR";
	error_desc[20095] = "DRV_INVALID_TRIGGER_MODE";
	error_desc[20096] = "DRV_LOAD_FIRMWARE_ERROR";
	error_desc[20097] = "DRV_DIVIDE_BY_ZERO_ERROR";
	error_desc[20098] = "DRV_INVALID_RINGEXPOSURES";
	error_desc[20099] = "DRV_BINNING_ERROR";
	error_desc[20100] = "DRV_INVALID_AMPLIFIER";
	error_desc[20101] = "DRV_INVALID_COUNTCONVERT_MODE";
	
	error_desc[20990] = "DRV_ERROR_NOCAMERA";
	error_desc[20991] = "DRV_NOT_SUPPORTED";
	error_desc[20992] = "DRV_NOT_AVAILABLE";
	
	error_desc[20115] = "DRV_ERROR_MAP";
	error_desc[20116] = "DRV_ERROR_UNMAP";
	error_desc[20117] = "DRV_ERROR_MDL";
	error_desc[20118] = "DRV_ERROR_UNMDL";
	error_desc[20119] = "DRV_ERROR_BUFFSIZE";
	error_desc[20121] = "DRV_ERROR_NOHANDLE";
	
	error_desc[20130] = "DRV_GATING_NOT_AVAILABLE";
	error_desc[20131] = "DRV_FPGA_VOLTAGE_ERROR";
	
	error_desc[20150] = "DRV_OW_CMD_FAIL";
	error_desc[20151] = "DRV_OWMEMORY_BAD_ADDR";
	error_desc[20152] = "DRV_OWCMD_NOT_AVAILABLE";
	error_desc[20153] = "DRV_OW_NO_SLAVES";
	error_desc[20154] = "DRV_OW_NOT_INITIALIZED";
	error_desc[20155] = "DRV_OW_ERROR_SLAVE_NUM";
	error_desc[20156] = "DRV_MSTIMINGS_ERROR";
	
	error_desc[20173] = "DRV_OA_NULL_ERROR";
	error_desc[20174] = "DRV_OA_PARSE_DTD_ERROR";
	error_desc[20175] = "DRV_OA_DTD_VALIDATE_ERROR";
	error_desc[20176] = "DRV_OA_FILE_ACCESS_ERROR";
	error_desc[20177] = "DRV_OA_FILE_DOES_NOT_EXIST";
	error_desc[20178] = "DRV_OA_XML_INVALID_OR_NOT_FOUND_ERROR";
	error_desc[20179] = "DRV_OA_PRESET_FILE_NOT_LOADED";
	error_desc[20180] = "DRV_OA_USER_FILE_NOT_LOADED";
	error_desc[20181] = "DRV_OA_PRESET_AND_USER_FILE_NOT_LOADED";
	error_desc[20182] = "DRV_OA_INVALID_FILE";
	error_desc[20183] = "DRV_OA_FILE_HAS_BEEN_MODIFIED";
	error_desc[20184] = "DRV_OA_BUFFER_FULL";
	error_desc[20185] = "DRV_OA_INVALID_STRING_LENGTH";
	error_desc[20186] = "DRV_OA_INVALID_CHARS_IN_NAME";
	error_desc[20187] = "DRV_OA_INVALID_NAMING";
	error_desc[20188] = "DRV_OA_GET_CAMERA_ERROR";
	error_desc[20189] = "DRV_OA_MODE_ALREADY_EXISTS";
	error_desc[20190] = "DRV_OA_STRINGS_NOT_EQUAL";
	error_desc[20191] = "DRV_OA_NO_USER_DATA";
	error_desc[20192] = "DRV_OA_VALUE_NOT_SUPPORTED";
	error_desc[20193] = "DRV_OA_MODE_DOES_NOT_EXIST";
	error_desc[20194] = "DRV_OA_CAMERA_NOT_SUPPORTED";
	error_desc[20195] = "DRV_OA_FAILED_TO_GET_MODE";
	
	error_desc[20211] = "DRV_PROCESSING_FAILED";
}

int AndorCam::initialize() {
	io.msg(IO_DEB2, "AndorCam::initialize()");
  unsigned int error;
	
	// Initialize camera
	char cfgdir[andordir.size()+1];
	snprintf(cfgdir, andordir.size()+1, "%s", andordir.c_str());
	error = Initialize(cfgdir);
	if (error != DRV_SUCCESS) return error;
	sleep(2); // From Andor SDK generic.cpp
	
	// Set custom properties
	int xpix=0, ypix=0;
	error = GetDetector(&xpix, &ypix);
	if (error != DRV_SUCCESS) return error;
	
	res.x = xpix; res.y = ypix;
	
	int tdepth=0;
	error = GetBitDepth(0, &tdepth);
	if (error != DRV_SUCCESS) return error;
	depth = conv_depth(tdepth);
	io.msg(IO_INFO, "AndorCam::AndorCam() GetDetector: %d x %d @ %d.", res.x, res.y, depth);
	
	// Cooling settings
	error = SetFanMode(0);							// 0 = turn fan on
	if (error != DRV_SUCCESS) return error;
	error = SetTemperature(-80);
	if (error != DRV_SUCCESS) return error;
	error = CoolerON();
	if (error != DRV_SUCCESS) return error;
	
	// Acquisition settings
	error = SetTriggerMode(0); 					// 0 = internal trigger
	if (error != DRV_SUCCESS) return error;
	error = SetAcquisitionMode(5);			// 5 = run till abort
	if (error != DRV_SUCCESS) return error;
	error = SetReadMode(4);							// 4 = read full image
	if (error != DRV_SUCCESS) return error;
	
	// Set cycle time as fast as possible
	error = SetKineticCycleTime(0.0);
	if (error != DRV_SUCCESS) return error;
	
	// Set image cropping (no cropping)
	error = SetImage(1, 1, 1, res.x, 1, res.y);
	if (error != DRV_SUCCESS) return error;
	
	// Query run mode parameters
	long bufsize=0;
	GetSizeOfCircularBuffer(&bufsize);
	io.msg(IO_INFO, "AndorCam::AndorCam() GetSizeOfCircularBuffer: %ld.", bufsize);

	return DRV_SUCCESS;
}

void AndorCam::on_message(Connection *const conn, string line) {
	string orig = line;
	string command = popword(line);
	bool parsed = true;
	
	if (command == "get") {							// get ...
		string what = popword(line);
		
		if (what == "cooling") {					// get cooling
			conn->addtag("cooling");
			conn->write(format("ok cooling %d", cool_info.target));
		} else if (what == "frametransfer") { // get frametransfer
			conn->addtag("frametransfer");
			cam_get_frametranfer();
		} else if (what == "temperature") { // get temperature
			int ctmp = cam_get_cooltemp();
			conn->addtag("cooling");
			conn->write(format("ok temperature %d", ctmp));
		} else
			parsed = false;
	} else if (command == "set") {			// set ...
		string what = popword(line);

		if (what == "cooling") {					// set cooling <temp>
			int temp = popint(line);
			conn->addtag("cooling");
			if (temp < cool_info.range[1] && temp > cool_info.range[0])
				cam_set_cooltarget(temp);
			else
				conn->write(format("error :temperature invalid, should be [%d, %d]", cool_info.range[0], cool_info.range[1]));
		} else if (what == "frametransfer") { // set frametransfer <bool>
			int ft = popint(line);
			conn->addtag("frametransfer");
			cam_set_frametranfer(ft);
		} else
			parsed = false;
	} else
		parsed = false;
	
	// If not parsed here, call parent
	if (parsed == false)
		Camera::on_message(conn, orig);
}

void AndorCam::do_restart() {
	io.msg(IO_INFO, "AndorCam::do_restart()");
}

void AndorCam::cam_set_shutter(const int status) {
	int ret;
	if (status == SHUTTER_OPEN)
		ret = SetShutter(1, 1, 50, 50);
	else
		ret = SetShutter(1, 2, 0, 0);

	if (ret != DRV_SUCCESS)
		io.msg(IO_WARN, "AndorCam::cam_handler(R) SetShutter: %s", error_desc[ret].c_str());
	else
		shutstat = status;
}

void AndorCam::cam_handler() { 
	io.msg(IO_DEB1, "AndorCam::cam_handler()");
	//pthread::setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS);	
	int ret=0;
	
	while (mode != Camera::OFF) {
		switch (mode) {
			case Camera::RUNNING:
				io.msg(IO_DEB1, "AndorCam::cam_handler() RUNNING");

				// Open shutter
				cam_set_shutter(SHUTTER_OPEN);

				// Start acquisition
				ret = StartAcquisition();
				if (ret != DRV_SUCCESS) {
					io.msg(IO_ERR, "AndorCam::cam_handler(R) StartAcquisition: %s", error_desc[ret].c_str());
					mode = Camera::ERROR;
					break;
				}
				
				while (mode == Camera::RUNNING) {
					int waitacq = 2500;
					// Wait for a new frame for maximum 'waitacq' ms
					ret = WaitForAcquisitionTimeOut(waitacq);
					
					if (ret == DRV_SUCCESS) {
						// Try to get new frame data						
						ret = GetMostRecentImage16(img_buffer.at(count % nframes), (unsigned long) (res.x * res.y));
						if (ret == DRV_SUCCESS) {
							void *queue_ret = cam_queue(img_buffer.at(count % nframes), img_buffer.at(count % nframes));
							
							//if (queue_ret != NULL)
							//	io.msg(IO_XNFO, "AndorCam::cam_handler(R) cam_queue returned old frame");
							//! @todo handle returned data?
						} else {
							io.msg(IO_WARN, "AndorCam::cam_handler(R) GetMostRecentImage16 error %s", error_desc[ret].c_str());
						}
					} else {
							io.msg(IO_WARN, "AndorCam::cam_handler(R) no new data in %d milliseconds? %s", waitacq, error_desc[ret].c_str());
						// Add short sleep before trying again, sometimes WaitForAcquisitionTimeOut() freaks out
						usleep(0.1*1E6);
					}

				}
				
				// Abort acquisition and close shutter
				AbortAcquisition();
				cam_set_shutter(SHUTTER_CLOSED);

				break;
			case Camera::SINGLE:
				io.msg(IO_DEB1, "AndorCam::cam_handler() SINGLE");
				//! @todo Implement data acquisition in Andor SDK
				mode = Camera::WAITING;
				break;
			case Camera::OFF:
				io.msg(IO_INFO, "AndorCam::cam_handler() OFF.");
			case Camera::WAITING:
				io.msg(IO_INFO, "AndorCam::cam_handler() WAITING.");

				// Abort acquisition
				ret = AbortAcquisition();
				if (ret != DRV_SUCCESS && ret != DRV_IDLE)
					io.msg(IO_WARN, "AndorCam::cam_handler(W) AbortAcquisition: %s", error_desc[ret].c_str());

				// Close shutter
				cam_set_shutter(SHUTTER_CLOSED);

				// We wait until the mode changed (for WAITING), or until the thread is canceled (for OFF)
				{
					io.msg(IO_INFO, "AndorCam::cam_handler(W) waiting...");
					pthread::mutexholder h(&mode_mutex);
					mode_cond.wait(mode_mutex);
				}
				break;
			case Camera::CONFIG:
				io.msg(IO_DEB1, "AndorCam::cam_handler(C) CONFIG");
				break;
			default:
				io.msg(IO_ERR, "AndorCam::cam_handler() UNKNOWN!");
				break;
		}
	}
	io.msg(IO_INFO, "AndorCam::cam_handler() complete, end");
	//! @todo quit thread properly here?
	//pthread::exit();
}

void AndorCam::cam_set_exposure(const double value) {
	int ret = 0;
	{
		pthread::mutexholder h(&cam_mutex);
		ret = SetExposureTime(value);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_exposure() failed to set exposure: %s", error_desc[ret].c_str());
	
	exposure = cam_get_exposure();
}

double AndorCam::cam_get_exposure() {
	cam_get_timings();
	return exposure;
}

void AndorCam::cam_get_timings() {
	int ret=0;
	float exp=0, acc=0, kin=0;
	{
		pthread::mutexholder h(&cam_mutex);
		ret = GetAcquisitionTimings(&exp, &acc, &kin);
	}
	
	if (ret != DRV_SUCCESS) {
		io.msg(IO_ERR, "AndorCam::cam_get_timings() failed to get timings: %s", error_desc[ret].c_str());
	} else {
		exposure = (double) exp;
		interval = (double) kin;
	}
}

void AndorCam::cam_set_interval(const double value) {
	int ret = 0;
	{
		pthread::mutexholder h(&cam_mutex);
		ret = SetKineticCycleTime(value);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_interval() failed to set kinetic cycle time: %s", error_desc[ret].c_str());
	
	interval = cam_get_interval();
}

double AndorCam::cam_get_interval() {
	cam_get_timings();
	return interval;
}

int AndorCam::cam_set_frametranfer(const int ft)  {
	int ret = SetFrameTransferMode(ft);
	if (ret != DRV_SUCCESS) 
		io.msg(IO_ERR, "AndorCam::AndorCam() SetFrameTransferMode error: %d, %s", ret, error_desc[ret].c_str());
	else
		frametransfer = ft;
	return cam_get_frametranfer();
}

int AndorCam::cam_get_frametranfer() const {
	net_broadcast(format("ok frametransfer %d", frametransfer), "frametransfer");

	return frametransfer;	
}

void AndorCam::cam_set_gain(const double value) {
	int ret = 0;
	io.msg(IO_DEB1, "AndorCam::cam_set_gain() %g", value);
	
	if (value < emgain_range[0] || value > emgain_range[1]) {
		io.msg(IO_WARN, "AndorCam::cam_set_gain() requested gain %g out of range [%d, %d]", 
					 value, emgain_range[0], emgain_range[1]);
	}
	
	{
		pthread::mutexholder h(&cam_mutex);
		ret = SetEMCCDGain((int) value);								// range depends on temperature
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_gain() failed to set gain: %s", error_desc[ret].c_str());
	
	gain = cam_get_gain();
}

void AndorCam::cam_set_gain_mode(const int mode) {
	int ret = 0;
	io.msg(IO_DEB1, "AndorCam::cam_set_gain_mode() %d", mode);

	{
		pthread::mutexholder h(&cam_mutex);
		ret = SetEMGainMode((int) mode);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_gain() failed to set gain mode: %s", error_desc[ret].c_str());

	// Also get new EM CCD gain range
	cam_get_gain_range(&emgain_range[0], &emgain_range[1]);
	
	gain = cam_get_gain();
}

double AndorCam::cam_get_gain() {
	int gain=0, ret=0;
	{
		pthread::mutexholder h(&cam_mutex);
		ret = GetEMCCDGain(&gain);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_get_gain() failed to get gain: %s", error_desc[ret].c_str());

	// Also get new EM CCD gain range
	cam_get_gain_range(&emgain_range[0], &emgain_range[1]);

	return (double) gain;
}

void AndorCam::cam_get_gain_range(int *gain_min, int *gain_max) {
	int min_g=0, max_g=0, ret=0;
	{
		pthread::mutexholder h(&cam_mutex);
		ret = GetEMGainRange(&min_g, &max_g);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_get_gain_range() failed to get gain range: %s", error_desc[ret].c_str());
	
	*gain_min = min_g;
	*gain_max = max_g;	
}

void AndorCam::cam_set_offset(const double /*value*/) {
	//pthread::mutexholder h(&cam_mutex);
	//! @todo Implement cam_set_offset in Andor SDK
}

double AndorCam::cam_get_offset() {
	//! @todo Implement cam_get_offset in Andor SDK
	return offset;
}

void AndorCam::cam_set_mode(const mode_t newmode) {
	pthread::mutexholder h(&cam_mutex);
	if (newmode == mode)
		return;
	
	switch (newmode) {
		case Camera::RUNNING:
		case Camera::SINGLE:
		case Camera::WAITING:
		case Camera::OFF:
			io.msg(IO_INFO, "AndorCam::cam_set_mode(%s) setting.", mode2str(newmode).c_str());
			mode = newmode;
		{
			pthread::mutexholder h(&mode_mutex);
			mode_cond.broadcast();
		}
			break;
		case Camera::CONFIG:
			io.msg(IO_INFO, "AndorCam::cam_set_mode(%s) mode not supported.", mode2str(newmode).c_str());
		default:
			io.msg(IO_WARN, "AndorCam::cam_set_mode(%s) mode unknown.", mode2str(newmode).c_str());
			break;
	}	
}

// !!!: Additional interal API functions go here

void AndorCam::cam_get_coolrange() {
	cam_get_coolrange(&(cool_info.range[0]), &(cool_info.range[1]));
}

void AndorCam::cam_get_coolrange(int *mintemp, int *maxtemp) {
	int ret=0, temp[2] = {0,0};
	{
		pthread::mutexholder h(&cam_mutex);
		ret = GetTemperatureRange(&(temp[0]), &(temp[1]));
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_get_coolrange() GetTemperatureRange: %s", error_desc[ret].c_str());
	
	*mintemp = temp[0];
	*maxtemp = temp[1];
	net_broadcast(format("ok coolrange %d %d", temp[0], temp[1]), "cooling");
}

bool AndorCam::cam_get_cooleron() {
	int ret=0, status=0;
	{
		pthread::mutexholder h(&cam_mutex);
		ret = IsCoolerOn(&status);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_get_cooleron() IsCoolerOn: %s", error_desc[ret].c_str());
	
	return (bool) status;
}

void AndorCam::cam_set_cooler(bool status) {
	if (status)
		CoolerOFF();
	else
		CoolerON();
	bool coolstat = cam_get_cooleron();
	net_broadcast(format("ok coolerstatus %d", coolstat), "cooling");
}

void AndorCam::cam_set_cooltarget(const int value) {
	int ret=0;
	
	{
		pthread::mutexholder h(&cam_mutex);
		ret = CoolerON();
	}
		
	if (ret != DRV_SUCCESS) {
		io.msg(IO_ERR, "AndorCam::cam_set_cooltarget() failed turn on cooler: %s", error_desc[ret].c_str());
		return;
	}
	
	cool_info.operating = cam_get_cooleron();
	
	{
		pthread::mutexholder h(&cam_mutex);
		ret = SetTemperature(value);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_cooltarget() failed turn on cooler: %s", error_desc[ret].c_str());
	
	// There is no function to query the target temperature, so store it manually here
	cool_info.target = value;
	
	// Also get new EM CCD gain range
	cam_get_gain_range(&emgain_range[0], &emgain_range[1]);
	
	net_broadcast(format("ok cooltarget %lf", cool_info.target), "cooling");
}

int AndorCam::cam_get_cooltemp() {
	return get_cooltempstat(0);
}

int AndorCam::cam_get_coolstatus() {
	return get_cooltempstat(1);
}

void AndorCam::cam_set_shift_speed(const int hs, const int vs, const int vamp) {
	pthread::mutexholder h(&cam_mutex);
	int ret = 0;
	
	//! @todo Check if values hs, vs and vamp are valid in the first place
	
	ret = SetHSSpeed(0, hs);									// 0: 10MHz, 1: 5MHz, 2: 3MHz, 3: 1MHz
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_shift_speed() SetHSSpeed failed: %s", error_desc[ret].c_str());
	
	ret = SetVSSpeed(vs);											// 0: 0.0875, 1: 0.1, 2: 0.15, 3: 0.25, 4: 0.45, may need to increase voltage amplitude to regain pixel well depth (iXonEM+ Hardware Guide 3.3.3)
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_shift_speed() SetVSSpeed failed: %s", error_desc[ret].c_str());
	
	ret = SetVSAmplitude(vamp);								// 0: normal, 1-4: increasing levels
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_set_shift_speed() SetVSAmplitude failed: %s", error_desc[ret].c_str());
}

void AndorCam::cam_get_capabilities() {
	io.msg(IO_DEB1, "AndorCam::cam_get_capabilities()");
	int ret = 0;
	
	caps.ulSize = sizeof(AndorCapabilities);
	{
		pthread::mutexholder h(&cam_mutex);
	  ret = GetCapabilities(&caps);
	}
	if (ret != DRV_SUCCESS)
		io.msg(IO_ERR, "AndorCam::cam_get_capabilities() failed: %s", error_desc[ret].c_str());
	else
		read_capabilities(&caps, caps_vec);
}

// !!!: Internal functions go here

int AndorCam::get_cooltempstat(int what) {
	pthread::mutexholder h(&cam_mutex);
	int ret=0, tmptemp=0;
	
	ret = GetTemperature(&tmptemp);
	
	if (ret != DRV_TEMP_OFF && 
			ret != DRV_TEMP_STABILIZED && 
			ret != DRV_TEMP_NOT_REACHED && 
			ret != DRV_TEMP_DRIFT && 
			ret != DRV_TEMP_NOT_STABILIZED)
		io.msg(IO_ERR, "AndorCam::get_cooltemp() failed get temp: %s", error_desc[ret].c_str());
	else
		cool_info.current = tmptemp;
	
	if (what == 0)
		return cool_info.current;
	else
		return ret;
}

void AndorCam::read_capabilities(AndorCapabilities *caps, std::vector< string> &cvec) {
	cvec.clear();
	
	cvec.push_back(format("caps.ulAcqModes for SetAcquisitionMode: %08x", caps->ulAcqModes));
	cvec.push_back(format("Single Scan Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_SINGLE) != 0 ));
	cvec.push_back(format("Video (Run Till Abort) Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_VIDEO) != 0 ));
	cvec.push_back(format("Accumulation Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_ACCUMULATE) != 0 ));
	cvec.push_back(format("Kinetic Series Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_KINETIC) != 0 ));
	cvec.push_back(format("Frame Transfer Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_FRAMETRANSFER) != 0 ));
	cvec.push_back(format("Fast Kinetics Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_FASTKINETICS) != 0 ));
	cvec.push_back(format("Overlap Acquisition Mode available using SetAcquisitionMode: %d", (caps->ulAcqModes & AC_ACQMODE_OVERLAP) != 0 ));
	
	cvec.push_back(format("caps.ulReadModes for SetReadMode: %08x", caps->ulReadModes));
	cvec.push_back(format("Full Image Read Mode available using SetReadMode: %d", (caps->ulReadModes & AC_READMODE_FULLIMAGE) != 0 ));
	cvec.push_back(format("Sub Image Read Mode available using SetReadMode: %d", (caps->ulReadModes & AC_READMODE_SUBIMAGE) != 0 ));
	cvec.push_back(format("Single track Read Mode available using SetReadMode: %d", (caps->ulReadModes & AC_READMODE_SINGLETRACK) != 0 ));
	cvec.push_back(format("Full Vertical Binning Read Mode available using SetReadMode: %d", (caps->ulReadModes & AC_READMODE_FVB) != 0 ));
	cvec.push_back(format("Multi Track Read Mode available using SetReadMode: %d", (caps->ulReadModes & AC_READMODE_MULTITRACK) != 0 ));
	cvec.push_back(format("Random­Track Read Mode available using SetReadMode: %d", (caps->ulReadModes & AC_READMODE_RANDOMTRACK) != 0 ));
	
	cvec.push_back(format("caps.ulFTReadModes for SetReadMode: %08x", caps->ulFTReadModes));
	cvec.push_back(format("Full Image Read Mode (Frame Transfer) available using SetReadMode: %d", (caps->ulFTReadModes & AC_READMODE_FULLIMAGE) != 0 ));
	cvec.push_back(format("Sub Image Read Mode (Frame Transfer) available using SetReadMode: %d", (caps->ulFTReadModes & AC_READMODE_SUBIMAGE) != 0 ));
	cvec.push_back(format("Single track Read Mode (Frame Transfer) available using SetReadMode: %d", (caps->ulFTReadModes & AC_READMODE_SINGLETRACK) != 0 ));
	cvec.push_back(format("Full Vertical Binning Read Mode (Frame Transfer) available using SetReadMode: %d", (caps->ulFTReadModes & AC_READMODE_FVB) != 0 ));
	cvec.push_back(format("Multi Track Read Mode (Frame Transfer) available using SetReadMode: %d", (caps->ulFTReadModes & AC_READMODE_MULTITRACK) != 0 ));
	cvec.push_back(format("Random­Track Read Mode (Frame Transfer) available using SetReadMode: %d", (caps->ulFTReadModes & AC_READMODE_RANDOMTRACK) != 0 ));
	
	cvec.push_back(format("caps.ulTriggerModes for SetTriggerMode: %08x", caps->ulTriggerModes));
	cvec.push_back(format("Internal Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_INTERNAL) != 0));
	cvec.push_back(format("External Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_EXTERNAL) != 0 ));
	cvec.push_back(format("External FVB EM Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_EXTERNAL_FVB_EM) != 0 ));
	cvec.push_back(format("Continuous (Software) Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_CONTINUOUS) != 0 ));
	cvec.push_back(format("External Start Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_EXTERNALSTART) != 0 ));
	cvec.push_back(format("Bulb Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_BULB) != 0 ));
	cvec.push_back(format("External Exposure Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_EXTERNALEXPOSURE) != 0 ));
	cvec.push_back(format("Inverted Trigger Mode available using SetTriggerMode: %d", (caps->ulTriggerModes & AC_TRIGGERMODE_INVERTED) != 0 ));
	
	// caps->ulCameraType is a integer indicating the camera type (0 -- 16)
	cvec.push_back(format("caps.ulCameraType: %08x", caps->ulCameraType));
	cvec.push_back(format("Camera Type: Andor PDA: %d", caps->ulCameraType == AC_CAMERATYPE_PDA));
	cvec.push_back(format("Camera Type: Andor iXon: %d", caps->ulCameraType == AC_CAMERATYPE_IXON));
	cvec.push_back(format("Camera Type: Andor ICCD: %d", caps->ulCameraType == AC_CAMERATYPE_ICCD));
	cvec.push_back(format("Camera Type: Andor EMCCD: %d", caps->ulCameraType == AC_CAMERATYPE_EMCCD));
	cvec.push_back(format("Camera Type: Andor CCD: %d", caps->ulCameraType == AC_CAMERATYPE_CCD));
	cvec.push_back(format("Camera Type: Andor iStar: %d", caps->ulCameraType == AC_CAMERATYPE_ISTAR));
	cvec.push_back(format("Camera Type: third party camera: %d", caps->ulCameraType == AC_CAMERATYPE_VIDEO));
	cvec.push_back(format("Camera Type: Andor iDus: %d", caps->ulCameraType == AC_CAMERATYPE_IDUS));
	cvec.push_back(format("Camera Type: Andor Newton: %d", caps->ulCameraType == AC_CAMERATYPE_NEWTON));
	cvec.push_back(format("Camera Type: Andor Surcam: %d", caps->ulCameraType == AC_CAMERATYPE_SURCAM));
	cvec.push_back(format("Camera Type: Andor USBiStar: %d", caps->ulCameraType == AC_CAMERATYPE_USBISTAR));
	cvec.push_back(format("Camera Type: Andor Luca: %d", caps->ulCameraType == AC_CAMERATYPE_LUCA));
	cvec.push_back(format("Camera Type: Andor Luca: %d", caps->ulCameraType == AC_CAMERATYPE_LUCA));
	cvec.push_back(format("Camera Type: Reserved: %d", caps->ulCameraType == AC_CAMERATYPE_RESERVED));
	cvec.push_back(format("Camera Type: Andor iKon: %d", caps->ulCameraType == AC_CAMERATYPE_IKON));
	cvec.push_back(format("Camera Type: Andor InGaAs: %d", caps->ulCameraType == AC_CAMERATYPE_INGAAS));
	cvec.push_back(format("Camera Type: Andor iVac: %d", caps->ulCameraType == AC_CAMERATYPE_IVAC));
	cvec.push_back(format("Camera Type: Andor Clara: %d", caps->ulCameraType == AC_CAMERATYPE_CLARA));
	
	cvec.push_back(format("caps.ulPixelMode: %08x", caps->ulPixelMode));
	cvec.push_back(format("Camera can acquire in 8­bit mode: %d", (caps->ulPixelMode & AC_PIXELMODE_8BIT) != 0 ));
	cvec.push_back(format("Camera can acquire in 14­bit mode: %d", (caps->ulPixelMode & AC_PIXELMODE_14BIT) != 0 ));
	cvec.push_back(format("Camera can acquire in 16­bit mode: %d", (caps->ulPixelMode & AC_PIXELMODE_16BIT) != 0 ));
	cvec.push_back(format("Camera can acquire in 32bit mode: %d", (caps->ulPixelMode & AC_PIXELMODE_32BIT) != 0 ));
	cvec.push_back(format("Camera can acquire in grey scale: %d", (caps->ulPixelMode & AC_PIXELMODE_MONO) != 0 ));
	cvec.push_back(format("Camera can acquire in RGB mode: %d", (caps->ulPixelMode & AC_PIXELMODE_RGB) != 0 ));
	cvec.push_back(format("Camera can acquire in CMY mode: %d", (caps->ulPixelMode & AC_PIXELMODE_CMY) != 0 ));
	
	cvec.push_back(format("caps.ulSetFunctions: %08x", caps->ulSetFunctions));
	cvec.push_back(format("SetVSSpeed: %d", (caps->ulSetFunctions & AC_SETFUNCTION_VREADOUT) != 0 ));
	cvec.push_back(format("SetHSSpeed: %d", (caps->ulSetFunctions & AC_SETFUNCTION_HREADOUT) != 0 ));
	cvec.push_back(format("SetTemperature: %d", (caps->ulSetFunctions & AC_SETFUNCTION_TEMPERATURE) != 0 ));
	cvec.push_back(format("SetMCPGain: %d", (caps->ulSetFunctions & AC_SETFUNCTION_MCPGAIN) != 0 ));
	cvec.push_back(format("SetEMCCDGain: %d", (caps->ulSetFunctions & AC_SETFUNCTION_EMCCDGAIN) != 0 ));
	cvec.push_back(format("SetBaselineClamp: %d", (caps->ulSetFunctions & AC_SETFUNCTION_BASELINECLAMP) != 0 ));
	cvec.push_back(format("SetVSAmplitude: %d", (caps->ulSetFunctions & AC_SETFUNCTION_VSAMPLITUDE) != 0 ));
	cvec.push_back(format("SetHighCapacity: %d", (caps->ulSetFunctions & AC_SETFUNCTION_HIGHCAPACITY) != 0 ));
	cvec.push_back(format("SetBaselineOffset: %d", (caps->ulSetFunctions & AC_SETFUNCTION_BASELINEOFFSET) != 0 ));
	cvec.push_back(format("SetPreAmpGain: %d", (caps->ulSetFunctions & AC_SETFUNCTION_PREAMPGAIN) != 0 ));
	cvec.push_back(format("SetCropMode/SetIsolatedCropMode: %d", (caps->ulSetFunctions & AC_SETFUNCTION_CROPMODE) != 0 ));
	cvec.push_back(format("SetDMAParameters: %d", (caps->ulSetFunctions & AC_SETFUNCTION_DMAPARAMETERS) != 0 ));
	cvec.push_back(format("Relative read mode horizontal binning: %d", (caps->ulSetFunctions & AC_SETFUNCTION_HORIZONTALBIN) != 0 ));
	cvec.push_back(format("SetMultiTrackHRange: %d", (caps->ulSetFunctions & AC_SETFUNCTION_MULTITRACKHRANGE) != 0 ));
	cvec.push_back(format("SetRandomTracks or SetComplexImage: %d", (caps->ulSetFunctions & AC_SETFUNCTION_RANDOMTRACKNOGAPS) != 0 ));
	cvec.push_back(format("SetEMAdvanced: %d", (caps->ulSetFunctions & AC_SETFUNCTION_EMADVANCED) != 0 ));
	
	cvec.push_back(format("caps.ulGetFunctions: %08x", caps->ulGetFunctions));
	cvec.push_back(format("GetTemperature: %d", (caps->ulGetFunctions & AC_GETFUNCTION_TEMPERATURE) != 0 ));
	cvec.push_back(format("GetTemperatureRange: %d", (caps->ulGetFunctions & AC_GETFUNCTION_TEMPERATURERANGE) != 0 ));
	cvec.push_back(format("GetDetector: %d", (caps->ulGetFunctions & AC_GETFUNCTION_DETECTORSIZE) != 0 ));
	cvec.push_back(format("AC_GETFUNCTION_MCPGAIN (reserved): %d", (caps->ulGetFunctions & AC_GETFUNCTION_MCPGAIN) != 0 ));
	cvec.push_back(format("GetEMCCDGain: %d", (caps->ulGetFunctions & AC_GETFUNCTION_EMCCDGAIN) != 0 ));
	cvec.push_back(format("GetBaselineClamp: %d", (caps->ulGetFunctions & AC_GETFUNCTION_BASELINECLAMP) != 0 ));
	
	cvec.push_back(format("caps.ulFeatures: %08x", caps->ulFeatures));
	cvec.push_back(format("GetStatus AC_FEATURES_POLLING: %d", (caps->ulFeatures & AC_FEATURES_POLLING) != 0 ));
	cvec.push_back(format("Windows Event AC_FEATURES_EVENTS: %d", (caps->ulFeatures & AC_FEATURES_EVENTS) != 0 ));
	cvec.push_back(format("SetSpool: %d", (caps->ulFeatures & AC_FEATURES_SPOOLING) != 0 ));
	cvec.push_back(format("SetShutter: %d", (caps->ulFeatures & AC_FEATURES_SHUTTER) != 0 ));
	cvec.push_back(format("SetShutterEx: %d", (caps->ulFeatures & AC_FEATURES_SHUTTEREX) != 0 ));
	cvec.push_back(format("Dedicated external I2C bus: %d", (caps->ulFeatures & AC_FEATURES_EXTERNAL_I2C) != 0 ));
	cvec.push_back(format("SetSaturationEvent: %d", (caps->ulFeatures & AC_FEATURES_SATURATIONEVENT) != 0 ));
	cvec.push_back(format("SetFanMode: %d", (caps->ulFeatures & AC_FEATURES_FANCONTROL) != 0 ));
	cvec.push_back(format("SetFanMode low fan setting: %d", (caps->ulFeatures & AC_FEATURES_MIDFANCONTROL) != 0 ));
	cvec.push_back(format("GetTemperature during acquisition: %d", (caps->ulFeatures & AC_FEATURES_TEMPERATUREDURINGACQUISITION) != 0 ));
	cvec.push_back(format("turn off keep cleans between scans: %d", (caps->ulFeatures & AC_FEATURES_KEEPCLEANCONTROL) != 0 ));
	cvec.push_back(format("AC_FEATURES_DDGLITE (reserved): %d", (caps->ulFeatures & AC_FEATURES_DDGLITE) != 0 ));
	cvec.push_back(format("Frame Transfer and External Exposure modes combination: %d", (caps->ulFeatures & AC_FEATURES_FTEXTERNALEXPOSURE) != 0 ));
	cvec.push_back(format("External Exposure trigger mode with Kinetic acquisition mode: %d", (caps->ulFeatures & AC_FEATURES_KINETICEXTERNALEXPOSURE) != 0 ));
	cvec.push_back(format("AC_FEATURES_DACCONTROL (reserved): %d", (caps->ulFeatures & AC_FEATURES_DACCONTROL) != 0 ));
	cvec.push_back(format("AC_FEATURES_METADATA (reserved): %d", (caps->ulFeatures & AC_FEATURES_METADATA) != 0 ));
	cvec.push_back(format("Configurable IO: %d", (caps->ulFeatures & AC_FEATURES_IOCONTROL) != 0 ));
	cvec.push_back(format("Photon counting: %d", (caps->ulFeatures & AC_FEATURES_PHOTONCOUNTING) != 0 ));
	cvec.push_back(format("Count Convert: %d", (caps->ulFeatures & AC_FEATURES_COUNTCONVERT) != 0 ));
	cvec.push_back(format("Dual exposure mode: %d", (caps->ulFeatures & AC_FEATURES_DUALMODE) != 0 ));
	
	cvec.push_back(format("caps.ulPCICard: %08x", caps->ulPCICard));
	cvec.push_back(format("Maximum PCI speed in Hz: %d", caps->ulPCICard));
	
	cvec.push_back(format("caps.ulEMGainCapability: %08x", caps->ulEMGainCapability));
	cvec.push_back(format("8bit DAC: %d", (caps->ulEMGainCapability & AC_EMGAIN_8BIT) != 0 ));
	cvec.push_back(format("12bit DAC: %d", (caps->ulEMGainCapability & AC_EMGAIN_12BIT) != 0 ));
	cvec.push_back(format("Gain setting linear: %d", (caps->ulEMGainCapability & AC_EMGAIN_LINEAR12) != 0 ));
	cvec.push_back(format("Gain setting real EM gain: %d", (caps->ulEMGainCapability & AC_EMGAIN_REAL12) != 0 ));
	
	// Gain settings (pre-amp and EMCCD)
	int i, npg, emh, eml;
	float pg;
	string preamp_gains = "";
	
	GetNumberPreAmpGains(&npg);
	for (i = 0; i < npg; i++) {
		GetPreAmpGain(i, &pg);
		if (i == 0)
			preamp_gains += format("%g", pg);
		else
			preamp_gains += format(", %g", pg);
	}
	
	cvec.push_back("Pre Amp Gain Factors: " + preamp_gains);
	
	GetEMGainRange(&eml, &emh);
	cvec.push_back(format("EMCCD Gain Range: %d -- %d", eml, emh));
	
	// Shift speeds (vertical, horizontal)
	int nvs, nhse, nhsc;
	float vss, hss;
	string vert_shifts = "", hor_shifts_em = "", hor_shifts_c = "";
	
	GetNumberVSSpeeds(&nvs);
	for (i = 0; i < nvs; i++) {
		GetVSSpeed(i, &vss);
		if (i == 0)
			vert_shifts += format("%g", vss);
		else
			vert_shifts += format(", %g", vss);
	}
	
	cvec.push_back("Vertical Shift Speeds: " + vert_shifts);
	
	GetNumberHSSpeeds(0,0,&nhse);
	GetNumberHSSpeeds(0,1,&nhsc);
	for (i = 0; i < nhse; i++) {
		GetHSSpeed(0, 0, i, &hss);
		if (i == 0)
			hor_shifts_em += format("%g", hss);
		else
			hor_shifts_em += format(", %g", hss);
	}
	
	cvec.push_back("Horizontal Shift Speeds (EM): " + hor_shifts_em);
	
	for (i = 0; i < nhsc; i++) {
		GetHSSpeed(1, 0, i, &hss);
		if (i == 0)
			hor_shifts_c += format("%g", hss);
		else
			hor_shifts_c += format(", %g", hss);
	}
	
	cvec.push_back("Horizontal Shift Speeds (C): " + hor_shifts_c);
}

// !!!: Additional Camera:: API functions go here

void AndorCam::print_andor_caps(FILE *fd) {
	for (size_t i=0; i < caps_vec.size(); i++) {
		fprintf(fd, "%s\n", caps_vec.at(i).c_str());
	}
}
