/*
 Copyright (C) 2008 Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 
 This file is part of FOAM.
 
 FOAM is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 FOAM is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with FOAM.  If not, see <http://www.gnu.org/licenses/>.

 $Id$
 */
/*! 
 @file foam_primemod-mcmath.c
 @author Tim van Werkhoven (t.i.m.vanwerkhoven@xs4all.nl)
 @date 2008-07-15
 
 @brief This is the McMath prime-module which can be used at that telescope.
 */

// HEADERS //
/***********/

// We need these for modMessage, see foam_cs.c
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;

//#define FOAM_MCMATH_DISPLAY 1
//#undef FOAM_MCMATH_DISPLAY

// GLOBALS //
/***********/

// Displaying
#ifdef FOAM_MCMATH_DISPLAY
mod_display_t disp;
#endif

// ITIFG camera & buffer
mod_itifg_cam_t dalsacam;
mod_itifg_buf_t buffer;

// DAQboard types
mod_daq2k_board_t daqboard;

// Okotech DM type
mod_okodm_t okodm;
// get some memory for fake DM actuator control
gsl_vector_float *dmctrl;
// actuators on the left and right
int okoleft[19] = {1, 2, 3, 7, 8, 9, 10, 11, 18, 19, 20, 21, 22, 23, 24, 34, 35, 36, 37};
int okoright[19] = {4, 5, 6, 12, 13, 14, 15, 16, 17, 25, 26, 27, 28, 29, 30, 31, 32, 33, 33};

// Shack Hartmann tracking info
mod_sh_track_t shtrack;

// Log some data here
mod_log_t shlog;
mod_log_t wfclog;

// Use an image buffer to store & dump images off the WFS
mod_imgbuf_t imgbuf;


int modInitModule(control_t *ptc, config_t *cs_config) {
	logInfo(0, "This is the McMath-Pierce prime module, enjoy.");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant initialliy
	ptc->logfrac = 100;                 // log verbose messages only every 100 frames
	ptc->wfs_count = 1;					// 2 FW, 1 WFS and 2 WFC
	ptc->wfc_count = 1;
	ptc->fw_count = 2;
	
	// allocate memory for filters, wfcs and wfss
	// use malloc to make the memory globally available
	ptc->filter = (filtwheel_t *) calloc(ptc->fw_count, sizeof(filtwheel_t));
	ptc->wfc = (wfc_t *) calloc(ptc->wfc_count, sizeof(wfc_t));
	ptc->wfs = (wfs_t *) calloc(ptc->wfs_count, sizeof(wfs_t));
	
	// configure WFS 0
	ptc->wfs[0].name = "SH WFS";
	ptc->wfs[0].res.x = 256;
	ptc->wfs[0].res.y = 256;
	ptc->wfs[0].bpp = 8;
	// this is where we will look for dark/flat/sky images
	ptc->wfs[0].darkfile = FOAM_DATADIR FOAM_CONFIG_PRE "_dark.gsldump";	
	ptc->wfs[0].flatfile = FOAM_DATADIR FOAM_CONFIG_PRE "_flat.gsldump";
	ptc->wfs[0].skyfile = FOAM_DATADIR FOAM_CONFIG_PRE "_sky.gsldump";	
	ptc->wfs[0].scandir = AO_AXES_XY;
	ptc->wfs[0].id = 0;
	ptc->wfs[0].fieldframes = 1000;     // take 1000 frames for a dark or flatfield
	
//	// configure WFC 0
//	ptc->wfc[0].name = "Okotech DM";
//	ptc->wfc[0].nact = 37;
//	ptc->wfc[0].gain.p = 1.0;
//	ptc->wfc[0].gain.i = 1.0;
//	ptc->wfc[0].gain.d = 1.0;
//	ptc->wfc[0].type = WFC_DM;
//    ptc->wfc[0].id = 0;
//	ptc->wfc[0].calrange[0] = -1.0;
//	ptc->wfc[0].calrange[1] = 1.0;	
	
	// configure WFC 0 (or 1), the TT
	ptc->wfc[0].name = "TT";
	ptc->wfc[0].nact = 2;
	ptc->wfc[0].gain.p = 1.0;
	ptc->wfc[0].gain.i = 1.0;
	ptc->wfc[0].gain.d = 1.0;
	ptc->wfc[0].type = WFC_TT;
    ptc->wfc[0].id = 1;
	ptc->wfc[0].calrange[0] = -1.0;
	ptc->wfc[0].calrange[1] = 1.0;	
	
	// configure filter 0
	ptc->filter[0].name = "Telescope FW";
	ptc->filter[0].id = 0;
    ptc->filter[0].delay = 2;
	ptc->filter[0].nfilts = 4;
	ptc->filter[0].filters[0] = FILT_PINHOLE;
	ptc->filter[0].filters[1] = FILT_OPEN;
    ptc->filter[0].filters[2] = FILT_TARGET;
    ptc->filter[0].filters[3] = FILT_CLOSED;
	
	ptc->filter[1].name = "WFS FW";
	ptc->filter[1].id = 1;
	ptc->filter[1].nfilts = 2;
    ptc->filter[1].delay = 2;
    ptc->filter[1].filters[0] = FILT_PINHOLE;
	ptc->filter[1].filters[1] = FILT_OPEN;
	
	// configure ITIFG camera & buffer
	dalsacam.module = 48;
	dalsacam.device_name = "/dev/ic0dma";
	dalsacam.config_file = FOAM_CONFDIR "dalsa-cad6-pcd.cam";
	
	buffer.frames = 8;
	
	// init itifg
	itifgInitBoard(&dalsacam);
	itifgInitBufs(&buffer, &dalsacam);
	
	// configure the daqboard
	
	daqboard.device = "daqBoard2k0";	// we use the first daqboard
	daqboard.nchans = 4;				// we use 4 analog chans [-10, 10] V
	daqboard.minvolt = -10.0;
	daqboard.maxvolt = 10.0;
	daqboard.iop2conf[0] = DAQ_OUTPUT;
	daqboard.iop2conf[1] = DAQ_OUTPUT;
	daqboard.iop2conf[2] = DAQ_INPUT;
	daqboard.iop2conf[3] = DAQ_INPUT;			// use digital IO ports for {out, out, in, in}
	
	// init Daq2k board, sets voltage to 0
	// The Daq2k board ranges from -10 to 10 volts, with 16 bit precision.
	// 0 results in -10V, 65535 in 10V, so that 32768 is 0V. 5V is thus 65535*3/4 = 49152
	daq2kInit(&daqboard);
	// since the TT works from 0V to 10V, set the Daq2k output to 5V for center
	daq2kSetDACs(&daqboard, (int) 32768 + 16384);
	
	// configure DM here
	
	okodm.minvolt = 0;					// nice voltage range is 0--255, middle is 180
	okodm.midvolt = 180;
	okodm.maxvolt = 255;
	okodm.nchan = 38;					// 37 acts + substrate = 38 channels
	okodm.port = "/dev/port";			// access pci board here
	okodm.pcioffset = 4;				// offset is 4 (sizeof(int)?)
	okodm.pcibase[0] = 0xc000;			// base addresses from lspci -v
	okodm.pcibase[1] = 0xc400;
	okodm.pcibase[2] = 0xffff;
	okodm.pcibase[3] = 0xffff;
	
	// init the mirror
	okoInitDM(&okodm);
	// allocate memory for fake ctrl
	dmctrl = gsl_vector_float_alloc(37);
	
	// shtrack configuring
    // we have a CCD of WxH, with a lenslet array of WlxHl, such that
    // each lenslet occupies W/Wl x H/Hl pixels, and we use track.x x track.y
    // pixels to track the CoG or do correlation tracking.
	shtrack.cells.x = 16;				// we're using a 16x16 lenslet array
	shtrack.cells.y = 16;
	shtrack.shsize.x = ptc->wfs[0].res.x/shtrack.cells.x;
	shtrack.shsize.y = ptc->wfs[0].res.y/shtrack.cells.y;
	shtrack.track.x = shtrack.shsize.x/2;   // tracker windows are half the size of the lenslet grid things
	shtrack.track.y = shtrack.shsize.y/2;
	shtrack.pinhole = FOAM_DATADIR FOAM_CONFIG_PRE "_pinhole.gsldump";
	shtrack.influence = FOAM_DATADIR FOAM_CONFIG_PRE "_influence.gsldump";
	shtrack.skipframes = 10;		// skip 10 frames before measureing
	shtrack.measurecount = 3;		// measure 10 times per actuator voltage
	shtrack.samxr = -4;			// 1 row edge erosion
	shtrack.samini = 20;			// minimum intensity for subaptselection 10
	// init the shtrack module for wfs 0 here
	shInit(&(ptc->wfs[0]), &shtrack);	
	
	// Configure some img buffering stuff
	imgbuf.imgres = ptc->wfs[0].res;
	imgbuf.imgsize = imgbuf.imgres.x * imgbuf.imgres.y;
	imgbuf.initalloc = imgbuf.imgsize * 750;
	imgInitBuf(&imgbuf);
	
	// Configure datalogging for SH offset measurements
	shlog.fname = "sh-offsets.dat";		// logfile name
	shlog.mode = "w";				// open with append mode (don't delete existing files)
	shlog.sep = " ";				// use space as a separator
	shlog.comm = "#";				// use a hash as comment char
	shlog.use = false;				// don't use the logfile immediately
	// Configure log for WFC signals ('voltages')
	wfclog.fname = "wfc-signals.dat";
	wfclog.mode = "w";
	wfclog.sep = " ";
	wfclog.comm = "#";	
	wfclog.use = false;
	
	// Init logging
	logInit(&shlog, ptc);
	logInit(&wfclog, ptc);
	
	// configure cs_config here
	cs_config->listenip = "0.0.0.0";	// listen on any IP by defaul
	cs_config->listenport = 10000;		// listen on port 10000 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam-mm";	// prepend logging with 'foam-mm'
	cs_config->use_stdout = true;		// do use stdout
	cs_config->loglevel = LOGDEBUG;		// log error, info and debug
	cs_config->infofile = NULL;			// don't log anything to file
	cs_config->errfile = NULL;
	cs_config->debugfile = NULL;

	
	return EXIT_SUCCESS;
}

int modPostInitModule(control_t *ptc, config_t *cs_config) {
	// we initialize OpenGL here, because it doesn't like threading
	// a lot
#ifdef FOAM_MCMATH_DISPLAY
	// init display
	disp.caption = "WFS #1";
	disp.res.x = ptc->wfs[0].res.x;
	disp.res.y = ptc->wfs[0].res.y;
	disp.autocontrast = 0;
	disp.brightness = 0;
	disp.contrast = 5;
	disp.dispsrc = DISPSRC_RAW;         // use the raw ccd output
	disp.dispover = DISPOVERLAY_GRID;   // display the SH grid
	disp.col.r = 255;
	disp.col.g = 255;
	disp.col.b = 255;
	
	displayInit(&disp);
#endif
	return EXIT_SUCCESS;
}

void modStopModule(control_t *ptc) {
#ifdef FOAM_MCMATH_DISPLAY
	displayFinish(&disp);
#endif
	
	// Stop ITIFG
	itifgStopGrab(&dalsacam);
	itifgStopBufs(&buffer, &dalsacam);
	itifgStopBoard(&dalsacam);
	
	// Stop the DAQboard
	daq2kClose(&daqboard);
	
	// Stop the Okotech DM
	okoCloseDM(&okodm);
}



// OPEN LOOP ROUTINES //
/*********************/

int modOpenInit(control_t *ptc) {
	// log mode change
	logMsg(&shlog, shlog.comm, "Init open loop", "\n");
	logMsg(&wfclog, shlog.comm, "Init open loop", "\n");
	logPTC(&shlog, ptc, shlog.comm);
	logPTC(&wfclog, ptc, shlog.comm);
	
	// start grabbing frames
	return itifgInitGrab(&dalsacam);
}

int modOpenLoop(control_t *ptc) {
	int i;
	static char title[64];
	char *fname;
	// get an image, without using a timeout
	if (drvGetImg(ptc, 0) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	// Full dark-flat correction
	MMDarkFlatFullByte(&(ptc->wfs[0]), &shtrack);
	
	// Track the CoG using the full-frame corrected image
	shCogTrack(ptc->wfs[0].corrim, DATA_GSL_M_F, ALIGN_RECT, &shtrack, NULL, NULL);
	
	// Move the DM mirror around, generate some noise TT signal with 50-frame
	// periodicty. Use sin and -sin to get nice signals
/*	for (i = 0; i<18; i++) {
		gsl_vector_float_set(dmctrl, okoleft[i], sin(ptc->frames *6.283 /50));
		gsl_vector_float_set(dmctrl, okoright[i], -sin(ptc->frames *6.283 /50));
	}
	okoSetDM(dmctrl, &okodm);
*/
	// log some data, prepend 'C' for closed loop
	logGSLVecFloat(&shlog, shtrack.disp, 2*shtrack.nsubap, "O", "\n");
	
	if (ptc->saveimg > 0) { // user wants to save images, do so now!		
		imgSaveToBuf(&imgbuf, ptc->wfs[0].image, DATA_UINT8, ptc->wfs[0].res);
		ptc->saveimg--;
		if (ptc->saveimg == 0) { // this was the last frame, report this
			//tellClients("200 FRAME CAPTURE COMPLETE");
			logInfo(0, "Frame capture complete, now dumping to disk");
			imgDumpBuf(&imgbuf, ptc);
		}
	}
	
#ifdef FOAM_MCMATH_DISPLAY
    if (ptc->frames % ptc->logfrac == 0) {
		displayDraw((&ptc->wfs[0]), &disp, &shtrack);
		displaySDLEvents(&disp);
		//logInfo(0, "Current framerate: %.2f FPS", ptc->fps);
		snprintf(title, 64, "%s (O) %.0f FPS", disp.caption, ptc->fps);
		SDL_WM_SetCaption(title, 0);
    }
#endif
	return EXIT_SUCCESS;
}

int modOpenFinish(control_t *ptc) {
	// stop grabbing frames
	return itifgStopGrab(&dalsacam);
}

// CLOSED LOOP ROUTINES //
/************************/

int modClosedInit(control_t *ptc) {
	// log mode change
	logMsg(&shlog, shlog.comm, "Init closed loop", "\n");
	logMsg(&wfclog, shlog.comm, "Init closed loop", "\n");
	logPTC(&shlog, ptc, shlog.comm);
	logPTC(&wfclog, ptc, shlog.comm);
	
	// set disp source to calib
	disp.dispsrc = DISPSRC_FASTCALIB;		
	// start grabbing frames
	return itifgInitGrab(&dalsacam);
}

int modClosedLoop(control_t *ptc) {
	static char title[64];
	int i;
	char *fname;
	// get an image, without using a timeout
	if (drvGetImg(ptc, 0) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	// Move the DM mirror around, generate some noise TT signal with 50-frame
	// periodicty. Use sin and -sin to get nice signals
	/*
	for (i = 0; i<18; i++) {
		gsl_vector_float_set(dmctrl, okoleft[i], sin(ptc->frames *6.283 / 300));
		gsl_vector_float_set(dmctrl, okoright[i], -sin(ptc->frames *6.283 / 300));
	}
	*/
	//okoSetDM(dmctrl, &okodm);
			
	// partial dark-flat correction
	MMDarkFlatSubapByte(&(ptc->wfs[0]), &shtrack);
	
	// Track the CoG using the partial corrected frame
	shCogTrack(ptc->wfs[0].corr, DATA_UINT8, ALIGN_SUBAP, &shtrack, NULL, NULL);
	
	// calculate the control signals
	shCalcCtrl(ptc, &shtrack, 0, -1);
	
	// set actuator
	drvSetActuator(ptc, 0);
	
	// log some data, prepend 'C' for closed loop
	logGSLVecFloat(&shlog, shtrack.disp, 2*shtrack.nsubap, "C", "\n");
	logGSLVecFloat(&wfclog, ptc->wfc[0].ctrl, -1, "C-DM", "\n");
	//logGSLVecFloat(&wfclog, dmctrl, -1, "C-DM", "\n")
	
	if (ptc->saveimg > 0) { // user wants to save images, do so now!
		imgSaveToBuf(&imgbuf, ptc->wfs[0].image, DATA_UINT8, ptc->wfs[0].res);
		ptc->saveimg--;
		if (ptc->saveimg == 0) { // this was the last frame, report this
			//tellClients("200 FRAME CAPTURE COMPLETE");
			logInfo(0, "Frame capture complete, now dumping to disk");
			imgDumpBuf(&imgbuf, ptc);
		}
	}	
    if (ptc->frames % ptc->logfrac == 0) {

		logInfo(0, "Subapt displacements:");
		for (i = 0; i < shtrack.nsubap; i++)
			logInfo(LOG_NOFORMAT, "(%.2f, %.2f) ", \
					gsl_vector_float_get(shtrack.disp, 2*i + 0), \
					gsl_vector_float_get(shtrack.disp, 2*i + 1));
		logInfo(LOG_NOFORMAT, "\n");
		
		logInfo(0, "Actuator signal for TT: (%.2f, %.2f)", \
				gsl_vector_float_get(ptc->wfc[0].ctrl, 0), \
				gsl_vector_float_get(ptc->wfc[0].ctrl, 1));
		
#ifdef FOAM_MCMATH_DISPLAY
		displayDraw((&ptc->wfs[0]), &disp, &shtrack);
		//logInfo(0, "Current framerate: %.2f FPS", ptc->fps);
		snprintf(title, 64, "%s (C) %.0f FPS", disp.caption, ptc->fps);
		SDL_WM_SetCaption(title, 0);
#endif
    }

	return EXIT_SUCCESS;
}

int modClosedFinish(control_t *ptc) {
	// stop grabbing frames
	return itifgStopGrab(&dalsacam);
}

// MISC ROUTINES //
/*****************/

int modCalibrate(control_t *ptc) {
	FILE *fieldfd;		// to open some files (dark, flat, ...)
	char title[64]; 	// for the window title
	int i, j, sn;
	wfs_t *wfsinfo = &(ptc->wfs[0]); // shortcut
	dispsrc_t oldsrc = disp.dispsrc; // store the old display source here since we might just have to show dark or flatfields
	int oldover = disp.dispover;	// store the old overlay here
	aomode_t oldmode = ptc->mode;		// store old mode (we might want to switch back)

	if (ptc->calmode == CAL_DARK) {
		// take dark frames, and average
		logInfo(0, "Starting darkfield calibration now");

		if (itifgInitGrab(&dalsacam) != EXIT_SUCCESS)
			return EXIT_FAILURE;

		MMAvgFramesByte(ptc, wfsinfo->darkim, &(ptc->wfs[0]), wfsinfo->fieldframes);

		if (itifgStopGrab(&dalsacam) != EXIT_SUCCESS)
			return EXIT_FAILURE;

		// saving image for later usage
		fieldfd = fopen(wfsinfo->darkfile, "w+");	
		if (!fieldfd)  {
			logWarn("Could not open darkfield storage file '%s', not saving darkfield (%s).", wfsinfo->darkfile, strerror(errno));
			return EXIT_SUCCESS;
		}
		gsl_matrix_float_fprintf(fieldfd, wfsinfo->darkim, "%.10f");
		fclose(fieldfd);
		logInfo(0, "Darkfield calibration done, and stored to disk.");
		// set new display settings to show the darkfield
		disp.dispsrc = DISPSRC_DARK;
		disp.dispover = 0;
		disp.autocontrast = 1;
		displayDraw(wfsinfo, &disp, &shtrack);
		snprintf(title, 64, "%s - Darkfield", disp.caption);
		SDL_WM_SetCaption(title, 0);
		// reset the display settings
		disp.dispsrc = oldsrc;
		disp.dispover = oldover;
	}
	else if (ptc->calmode == CAL_FLAT) {
		logInfo(0, "Starting flatfield calibration now");
		// take flat frames, and average

		if (itifgInitGrab(&dalsacam) != EXIT_SUCCESS) 
			return EXIT_FAILURE;


		MMAvgFramesByte(ptc, wfsinfo->flatim, &(ptc->wfs[0]), wfsinfo->fieldframes);
		

		if (itifgStopGrab(&dalsacam) != EXIT_SUCCESS)
			return EXIT_FAILURE;

		// saving image for later usage
		fieldfd = fopen(wfsinfo->flatfile, "w+");	
		if (!fieldfd)  {
			logWarn("Could not open flatfield storage file '%s', not saving flatfield (%s).", wfsinfo->flatfile, strerror(errno));
			return EXIT_SUCCESS;
		}
		gsl_matrix_float_fprintf(fieldfd, wfsinfo->flatim, "%.10f");
		fclose(fieldfd);
		logInfo(0, "Flatfield calibration done, and stored to disk.");
		// set new display settings to show the darkfield
		disp.dispsrc = DISPSRC_FLAT;
		disp.dispover = 0;
		disp.autocontrast = 1;
		displayDraw(wfsinfo, &disp, &shtrack);
		snprintf(title, 64, "%s - Flatfield", disp.caption);
		SDL_WM_SetCaption(title, 0);
		// reset the display settings
		disp.dispsrc = oldsrc;
		disp.dispover = oldover;
	}
	else if (ptc->calmode == CAL_DARKGAIN) {
		logInfo(0, "Taking dark and flat images to make convenient images to correct (dark/gain).");
		
		// get the average flat-dark value for all subapertures (but not the whole image)
		float tmpavg, pix;
		for (sn=0; sn < shtrack.nsubap; sn++) {
			for (i=0; i< shtrack.track.y; i++) {
				for (j=0; j< shtrack.track.x; j++) {
					pix = (gsl_matrix_float_get(wfsinfo->flatim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j) - \
						gsl_matrix_float_get(wfsinfo->darkim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j));
					if (pix < 0) pix = 0;
					tmpavg += pix;
				}
			}
		}
		tmpavg /= (shtrack.nsubap * shtrack.track.x * shtrack.track.y);
		logDebug(0,"tmpavg: %f", tmpavg);
		
		// make actual matrices from dark and flat
		uint16_t *darktmp = (uint16_t *) wfsinfo->dark;
		uint16_t *gaintmp = (uint16_t *) wfsinfo->gain;

		for (sn=0; sn < shtrack.nsubap; sn++) {
			for (i=0; i< shtrack.track.y; i++) {
				for (j=0; j< shtrack.track.x; j++) {
					// 16 bit dark used for calculations is the darkfield * 256
					darktmp[sn * (shtrack.track.x * shtrack.track.y) + i*shtrack.track.x + j] = \
						(uint16_t) (256.0 * gsl_matrix_float_get(wfsinfo->darkim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j));
					// gain = avg(flat-dark)/flat-dark, but if flat-dark is 0, 
					// we set gain to 0 (this pixel is not useful I guess)
					// if 256* avg(flat-dark) / (flat-dark) is bigger than 2^16,
					// wrap it to 2^16-1 (i.e. saturated arithmetic)
					pix = (gsl_matrix_float_get(wfsinfo->flatim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j) - \
							  gsl_matrix_float_get(wfsinfo->darkim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j));
					if (pix <= 0)
						gaintmp[sn * (shtrack.track.x * shtrack.track.y) + i*shtrack.track.x + j] = 0;
					else 
						gaintmp[sn * (shtrack.track.x * shtrack.track.y) + i*shtrack.track.x + j] = (uint16_t) \
						fminf((256.0 * tmpavg / pix), 65535);
				}
			}
		}
		float darkst[3];
		float gainst[3];
		darkst[0] = darkst[1] = darkst[2] = 0;
		imgGetStats(wfsinfo->dark, DATA_UINT16, NULL, shtrack.nsubap * shtrack.track.x * shtrack.track.y, darkst);
		imgGetStats(wfsinfo->gain, DATA_UINT16, NULL, shtrack.nsubap * shtrack.track.x * shtrack.track.y, gainst);
		logDebug(0, "dark: min: %f, max: %f, avg: %f", darkst[0], darkst[1], darkst[2]);
		logDebug(0, "gain: min: %f, max: %f, avg: %f", gainst[0], gainst[1], gainst[2]);

		logInfo(0, "Dark and gain fields initialized");
	}
	else if (ptc->calmode == CAL_SUBAPSEL) {
		logInfo(0, "Starting subaperture selection now");
		// init grabbing

		if (itifgInitGrab(&dalsacam) != EXIT_SUCCESS) 
			return EXIT_FAILURE;

		// get a single image
//		drvGetImg(&dalsacam, &buffer, NULL, &(wfsinfo->image));
		drvGetImg(ptc, 0);
		
		// stop grabbing

		if (itifgStopGrab(&dalsacam) != EXIT_SUCCESS)
			return EXIT_FAILURE;

		uint8_t *tmpimg = (uint8_t *) wfsinfo->image;
		int tmpmax = tmpimg[0];
		int tmpmin = tmpimg[0];
		int tmpsum=0, i;
		for (i=0; i<wfsinfo->res.x*wfsinfo->res.y; i++) {
			tmpsum += tmpimg[i];
			if (tmpimg[i] > tmpmax) tmpmax = tmpimg[i];
			else if (tmpimg[i] < tmpmin) tmpmin = tmpimg[i];
		}           
		logInfo(0, "Image info: sum: %d, avg: %f, range: (%d,%d)", tmpsum, (float) tmpsum / (wfsinfo->res.x*wfsinfo->res.y), tmpmin, tmpmax);

		// run subapsel on this image
		//modSelSubapsByte((uint8_t *) wfsinfo->image, &shtrack, wfsinfo);
		shSelSubapts(wfsinfo->image, DATA_UINT8, ALIGN_RECT, &shtrack, wfsinfo);
			//shSelSubaptsByte(void *image, mod_sh_track_t *shtrack, wfs_t *shwfs, int *totnsubap, float samini, int samxr) 

	
		logInfo(0, "Subaperture selection complete, found %d subapertures.", shtrack.nsubap);
		// set new display settings to show the darkfield
		disp.dispsrc = DISPSRC_RAW;
		disp.dispover = DISPOVERLAY_SUBAPS | DISPOVERLAY_GRID;
		disp.autocontrast = 1;
		displayDraw(wfsinfo, &disp, &shtrack);
		snprintf(title, 64, "%s - Subaps", disp.caption);
		SDL_WM_SetCaption(title, 0);
		// reset the display settings
		disp.dispsrc = oldsrc;
		disp.dispover = oldover;
	}
	else if (ptc->calmode == CAL_PINHOLE) {
		// pinhole calibration to get reference coordinates
		logInfo(0, "Starting pinhole calibration to get reference coordinates now");
		
		calibPinhole(ptc, 0, &shtrack);
	}
	else if (ptc->calmode == CAL_INFL) {
		// influence function calibration
		logInfo(0, "Starting influence function calibration");
		
		calibWFC(ptc, 0, &shtrack);
	}
	
	return EXIT_SUCCESS;
}

int modMessage(control_t *ptc, const client_t *client, char *list[], const int count) {
	// Quick recap of messaging codes:
	// 400 UNKNOWN
	// 401 UNKNOWN MODE
	// 402 MODE REQUIRES ARG
	// 403 MODE FORBIDDEN
	// 300 ERROR
	// 200 OK 
	
	// Some useful variables
	int tmpint;
	float tmpfloat;
	long tmplong;
	// Time and date for logging (asctime, anyone?)
	time_t t = time(NULL);
	struct tm *localt = localtime(&t);
	
	
 	if (strncmp(list[0],"help",3) == 0) {
		// give module specific help here
		if (count > 1) { 
			
			if (strncmp(list[1], "disp",3) == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP DISPLAY\n\
display <source>:       change the display source.\n\
   <sources:>\n\
   raw:                 direct images from the camera.\n\
   cfull:               full dark/flat corrected images.\n\
   cfast:               fast partial dark/flat corrected images.\n\
   dark:                show the darkfield being used.\n\
   flat:                show the flatfield being used.\n\
   <overlays:>\n\
   subap:               toggle displat of the subapertures.\n\
   grid:                toggle display of the grid.\n\
   vecs:                toggle display of the displacement vectors.\n\
   labels:              toggle display of the subapt labels.\n\
   col [f] [f] [f]:     change the overlay color (OpenGL only).\
");
			}
			else if (strncmp(list[1], "vid",3) == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP VID\n\
vid <mode> [val]:       configure the video output.\n\
   auto:                use auto contrast/brightness.\n\
   c [i]:               use manual c/b with this contrast.\n\
   b [i]:               use manual c/b with this brightness.\
");
			}
			else if (strncmp(list[1], "set",3) == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP SET\n\
set [prop] [val]:       set or query property values.\n\
   lf [i]:              set the logfraction.\n\
   ff [i]:              set the number of frames to use for dark/flats.\n\
   samini [f]:          set the minimum intensity for subapt selection.\n\
   samxr [i]:           set maxr used for subapt selection.\n\
   -:                   if no prop is given, query the values.\
");
			}
			else if (strncmp(list[1], "gain",3) == 0) {
tellClient(client->buf_ev, "\
200 OK HELP GAIN\n\
   prop [wfc] [f]       set proportional gain for [wfc].\n\
   int [wfc] [f]        set integral gain for [wfc].\n\
   diff [wfc] [f]       set differential gain for [wfc].");
			}			
			else if (strncmp(list[1], "cal",3) == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP CALIBRATE\n\
calibrate <mode>:       calibrate the ao system.\n\
   dark:                take a darkfield by averaging %d frames.\n\
   flat:                take a flatfield by averaging %d frames.\n\
   gain:                calc dark/gain to do actual corrections with.\n\
   subap:               select some subapertures.\n\
   pinhole:             get reference coordinates which define a flat WF.\n\
   influence:           get the influence function for WFS 0 and all WFC's.\
", ptc->wfs[0].fieldframes, ptc->wfs[0].fieldframes);
			}
			else // we don't know. tell this to parseCmd by returning 0
				return 0;
		}
		else {
			tellClient(client->buf_ev, "\
=== prime module options ===\n\
display <source>:       tell foam what display source to use.\n\
vid <auto|c|v> [i]:     use autocontrast/brightness, or set manually.\n\
log [on|off|reset]:     toggle data logging on or off, or reset the logfiles\n\
resetdm [i]:            reset the DM to a certain voltage for all acts. def=0\n\
resetdaq [i]:           reset the DAQ analog outputs to a certain voltage. def=0\n\
set [prop]:             set or query certain properties.\n\
saveimg [i]:            buffer & dump the next i frames to disk.\n\
calibrate <mode>:       calibrate the ao system (dark, flat, subapt, etc).\
");
		}
	}
#ifdef FOAM_MCMATH_DISPLAY
 	else if (strncmp(list[0], "disp",3) == 0) {
		if (count > 1) {
			if (strncmp(list[1], "raw",3) == 0) {
				tellClients("200 OK DISPLAY RAW");
				disp.dispsrc = DISPSRC_RAW;
			}
			else if (strncmp(list[1], "cfull",3) == 0) {
				disp.dispsrc = DISPSRC_FULLCALIB;
				tellClients("200 OK DISPLAY CALIB");
			}
			else if (strncmp(list[1], "cfast",3) == 0) {
				disp.dispsrc = DISPSRC_FASTCALIB;
				tellClients("200 OK DISPLAY CALIB");
			}
			else if (strncmp(list[1], "grid",3) == 0) {
				logDebug(0, "overlay was: %d, is: %d, mask: %d", disp.dispover, disp.dispover ^ DISPOVERLAY_GRID, DISPOVERLAY_GRID);
				disp.dispover ^= DISPOVERLAY_GRID;
				tellClients("200 OK TOGGLING GRID OVERLAY");
			}
			else if (strncmp(list[1], "subaps",3) == 0) {
				disp.dispover ^= DISPOVERLAY_SUBAPS;
				tellClients("200 OK TOGGLING SUBAPERTURE OVERLAY");
			}
			else if (strncmp(list[1], "vectors",3) == 0) {
				disp.dispover ^= DISPOVERLAY_VECTORS;
				tellClients("200 OK TOGGLING DISPLACEMENT VECTOR OVERLAY");
			}
			else if (strncmp(list[1], "labels",3) == 0) {
				disp.dispover ^= DISPOVERLAY_SUBAPLABELS;
				tellClients("200 OK TOGGLING SUBAPERTURE LABELS");
			}
			else if (strncmp(list[1], "col",3) == 0) {
				if (count > 4) {
					disp.col.r = strtol(list[2], NULL, 10);
					disp.col.g = strtol(list[3], NULL, 10);
					disp.col.b = strtol(list[4], NULL, 10);
					tellClients("200 OK COLOR IS NOW (%d,%d,%d)", disp.col.r, disp.col.g, disp.col.b);
				}
				else {
					tellClient(client->buf_ev, "402 COLOR REQUIRES RGB FLOAT TRIPLET");
				}
			}
			else if (strncmp(list[1], "dark",3) == 0) {
				if  (ptc->wfs[0].darkim == NULL) {
					tellClient(client->buf_ev, "400 ERROR DARKFIELD NOT AVAILABLE");
				}
				else {
					disp.dispsrc = DISPSRC_DARK;
					tellClients("200 OK DISPLAY DARK");
				}
			}
			else if (strncmp(list[1], "flat",3) == 0) {
				if  (ptc->wfs[0].flatim == NULL) {
					tellClient(client->buf_ev, "400 ERROR FLATFIELD NOT AVAILABLE");
				}
				else {
					disp.dispsrc = DISPSRC_FLAT;
					tellClients("200 OK DISPLAY FLAT");
				}
			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN DISPLAY");
			}
		}
		else {
			tellClient(client->buf_ev, "200 OK DISPLAY INFO\n\
brightness:             %d\n\
contrast:               %f\n\
overlay:                %d\n\
source:                 %d", disp.brightness, disp.contrast, disp.dispover, disp.dispsrc);
		}
	}
#endif
	else if (strcmp(list[0],"saveimg") == 0) {
		if (count > 1) {
			tmplong = strtol(list[1], NULL, 10);
			ptc->saveimg = tmplong;
			tellClients("200 OK SAVING NEXT %ld IMAGES", tmplong);
		}
		else {
			tellClient(client->buf_ev,"402 SAVEIMG REQUIRES ARG (# FRAMES)");
		}		
	}	
	else if (strcmp(list[0], "log") == 0) {
		if (count > 1) {
			if (strcmp(list[1], "on") == 0) {
				shlog.use = true;
				wfclog.use = true;
				fprintf(shlog.fd, "%s Logging started at %s", shlog.comm, asctime(localt));
				fprintf(wfclog.fd, "%s Logging started at %s", shlog.comm, asctime(localt));
				tellClients("200 OK ENABLED DATA LOGGING");
			}
			else if (strcmp(list[1], "off") == 0) {
				fprintf(shlog.fd, "%s Logging stopped at %s", shlog.comm, asctime(localt));
				fprintf(wfclog.fd, "%s Logging stopped at %s", shlog.comm, asctime(localt));
				shlog.use = false;
				wfclog.use = false;
				tellClients("200 OK DISBLED DATA LOGGING");
			}
			else if (strcmp(list[1], "reset") == 0) {
				logReset(&shlog, ptc);
				logReset(&wfclog, ptc);
				tellClients("200 OK RESET DATA LOGGING");
			}
			else {
				tellClient(client->buf_ev,"401 UNKNOWN LOG COMMAND (on, off, reset)");
			}
		}	
		else {
			tellClient(client->buf_ev,"402 LOG REQUIRES ARG (on, off, reset)");
		}
	}
 	else if (strcmp(list[0], "resetdm") == 0) {
		if (count > 1) {
			tmpint = strtol(list[1], NULL, 10);
			
			if (tmpint >= okodm.minvolt && tmpint <= okodm.maxvolt) {
				if (okoSetAllDM(&okodm, tmpint) == EXIT_SUCCESS)
					tellClients("200 OK RESETDM %dV", tmpint);
				else
					tellClient(client->buf_ev, "300 ERROR RESETTING DM");
				
			}
			else {
				tellClient(client->buf_ev, "403 INCORRECT VOLTAGE!");
			}
		}
		else {
			if (okoRstDM(&okodm) == EXIT_SUCCESS)
				tellClients("200 OK RESETDM 0V");
			else 
				tellClient(client->buf_ev, "300 ERROR RESETTING DM");
			
		}
	}
 	else if (strcmp(list[0], "resetdaq") == 0) {
		if (count > 1) {
			tmpfloat = strtof(list[1], NULL);
			
			if (tmpfloat >= daqboard.minvolt && tmpfloat <= daqboard.maxvolt) {
				daq2kSetDACs(&daqboard, (int) 65536*(tmpfloat-daqboard.minvolt)/(daqboard.maxvolt-daqboard.minvolt) -1);
				tellClients("200 OK RESETDAQ %.2fV", tmpfloat);
			}
			else {
				tellClient(client->buf_ev, "403 INCORRECT VOLTAGE!");
			}
		}
		else {
			daq2kSetDACs(&daqboard, 65536*3/4);
			tellClients("200 OK RESETDAQ %.2fV", 5.0);			
		}
	}
 	else if (strncmp(list[0], "gain",3) == 0) {
		if (count > 3) {
			tmpint = strtol(list[2], NULL, 10);
			tmpfloat = strtof(list[3], NULL);
			if (tmpint >= 0 && tmpint < ptc->wfc_count && tmpfloat >= -1.0 && tmpfloat <= 1.0) {
				if (strncmp(list[1], "prop",3) == 0) {
					ptc->wfc[tmpint].gain.p = tmpfloat;
					tellClients( "200 OK SET PROP GAIN FOR WFC %d TO %.2f", tmpint, tmpfloat);
					logMsg(&shlog, shlog.comm, "GAIN: Changed prop gain, PTC dump follows", "\n");
					logPTC(&shlog, ptc, shlog.comm);
				}
				else if (strncmp(list[1], "diff",3) == 0) {
					ptc->wfc[tmpint].gain.d = tmpfloat;
					tellClients( "200 OK SET DIFF GAIN FOR WFC %d TO %.2f", tmpint, tmpfloat);
					logMsg(&shlog, shlog.comm, "GAIN: Changed diff gain, PTC dump follows", "\n");
					logPTC(&shlog, ptc, shlog.comm);					
				}
				else if (strncmp(list[1], "int",3) == 0) {
					ptc->wfc[tmpint].gain.i = tmpfloat;
					tellClients( "200 OK SET INT GAIN FOR WFC %d TO %.2f", tmpint, tmpfloat);
					logMsg(&shlog, shlog.comm, "GAIN: Changed int gain, PTC dump follows", "\n");
					logPTC(&shlog, ptc, shlog.comm);
				}
				else {
					tellClient(client->buf_ev, "401 UNKNOWN GAINTYPE");
				}
			}
			else {
				tellClient(client->buf_ev, "403 INCORRECT WFC OR GAIN VALUE");
			}
		}
		else {
			tellClient(client->buf_ev, "402 GAIN REQUIRES ARGS");
		}
	}		
 	else if (strncmp(list[0], "set",3) == 0) {
		if (count > 2) {
			tmpint = strtol(list[2], NULL, 10);
			tmpfloat = strtof(list[2], NULL);
			if (strcmp(list[1], "lf") == 0) {
				ptc->logfrac = tmpint;
				tellClients( "200 OK SET LOGFRAC TO %d", tmpint);
			}
			else if (strcmp(list[1], "ff") == 0) {
				ptc->wfs[0].fieldframes = tmpint;
				tellClients( "200 OK SET FIELDFRAMES TO %d", tmpint);
			}
			else if (strcmp(list[1], "samini") == 0) {
				shtrack.samini = tmpfloat;
				tellClients( "200 OK SET SAMINI TO %.2f", tmpfloat);
			}
			else if (strcmp(list[1], "samxr") == 0) {
				shtrack.samxr = tmpint;
				tellClients( "200 OK SET SAMXR TO %d", tmpint);
			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN PROPERTY, CANNOT SET");
			}
		}
		else {
			tellClient(client->buf_ev, "200 OK VALUES AS FOLLOWS:\n\
logfrac (lf):           %d\n\
fieldframes (ff):       %d\n\
SH array:               %dx%d cells\n\
cell size:              %dx%d pixels\n\
track size:             %dx%d pixels\n\
ccd size:               %dx%d pixels\n\
samxr:                  %d\n\
samini:                 %.2f\
", ptc->logfrac, ptc->wfs[0].fieldframes, shtrack.cells.x, shtrack.cells.y,\
shtrack.shsize.x, shtrack.shsize.y, shtrack.track.x, shtrack.track.y, ptc->wfs[0].res.x, ptc->wfs[0].res.y, shtrack.samxr, shtrack.samini);
		}
	}
 	else if (strncmp(list[0], "step",3) == 0) {
		if (count > 2) {
			tmpfloat = strtof(list[2], NULL);
			if (strncmp(list[1], "x",3) == 0) {
				shtrack.stepc.x = tmpfloat;
				tellClients( "200 OK STEP X %+f", tmpfloat);
			}
			else if (strcmp(list[1], "y") == 0) {
				shtrack.stepc.y = tmpfloat;
				tellClients( "200 OK STEP Y %+f", tmpfloat);
			}
		}
		else {
tellClient(client->buf_ev, "200 OK STEP INFO\n\
step (x,y):             (%+f, %+f)", shtrack.stepc.x, shtrack.stepc.y);	
		}
	}	
 	else if (strncmp(list[0], "vid",3) == 0) {
		if (count > 1) {
			if (strncmp(list[1], "auto",3) == 0) {
				disp.autocontrast = 1;
				tellClients( "200 OK USING AUTO SCALING");
			}
			else if (strcmp(list[1], "c") == 0) {
				if (count > 2) {
					tmpfloat = strtof(list[2], NULL);
					disp.autocontrast = 0;
					disp.contrast = tmpfloat;
					tellClients( "200 OK CONTRAST %f", tmpfloat);
				}
				else {
					tellClient(client->buf_ev, "402 NO CONTRAST GIVEN");
				}
			}
			else if (strcmp(list[1], "b") == 0) {
				if (count > 2) {
					tmpint = strtol(list[2], NULL, 10);
					disp.autocontrast = 0;
					disp.brightness = tmpint;
					tellClients( "200 OK BRIGHTNESS %d", tmpint);
				}
				else {
					tellClient(client->buf_ev, "402 NO BRIGHTNESS GIVEN");
				}
			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN VID");
			}
		}
		else {
tellClient(client->buf_ev, "200 OK VID INFO\n\
brightness:             %d\n\
contrast:               %f", disp.brightness, disp.contrast);
	
		}
	}
 	else if (strncmp(list[0], "cal",3) == 0) {
		if (count > 1) {
			if (strncmp(list[1], "dark",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_DARK;
                		tellClients( "200 OK DARKFIELDING NOW");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "subap",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_SUBAPSEL;
                		tellClients( "200 OK SELECTING SUBAPTS");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "flat",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_FLAT;
				tellClients( "200 OK FLATFIELDING NOW");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "gain",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_DARKGAIN;
				tellClients( "200 OK CALCULATING DARK/GAIN NOW");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "pinhole",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_PINHOLE;
                tellClients( "200 OK GETTING REFERENCE COORDINATES");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "influence",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_INFL;
                tellClients( "200 OK GETTING INFLUENCE FUNCTION");
				pthread_cond_signal(&mode_cond);
			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN CALIBRATION");
			}
		}
		else {
			tellClient(client->buf_ev, "402 CALIBRATE REQUIRES ARGS");
		}
	}
	else { // no valid command found? return 0 so that the main thread knows this
		return 0;
	} // strcmp stops here
	
	// if we end up here, we didn't return 0, so we found a valid command
	return 1;
}

// SITE-SPECIFIC ROUTINES //
/**************************/

int drvGetImg(control_t *ptc, int wfs) {

	// we're using an itifg driver, fetch an image to wfs 0
	if (wfs == 0)
		return itifgGetImg(&dalsacam, &buffer, NULL, &(ptc->wfs[0].image));
	
	// if we get here, the WFS is unknown
	return EXIT_FAILURE;
}

int drvSetActuator(control_t *ptc, int wfc) {

	if (ptc->wfc[wfc].type == WFC_TT) {			// Tip-tilt
		// use daq, scale [-1, 1] to 2^15--2^16 (which gives 0 to 10 volts
		// as output). TT is on ports 0 and 1
		// 32768 = 2^15, 16384 = 2^14, ([-1,1] + 1) * 16384 = [0,32768]
		// -1 to prevent overflow for ctrl = 1
		daq2kSetDAC(&daqboard, 0, (int) 32768 + (gsl_vector_float_get(ptc->wfc[wfc].ctrl, 0)+1) * 16384 - 1);
		daq2kSetDAC(&daqboard, 1, (int) 32768 + (gsl_vector_float_get(ptc->wfc[wfc].ctrl, 1)+1) * 16384 - 1);
		return EXIT_SUCCESS;
	}
	else if (ptc->wfc[wfc].type == WFC_DM) {
		/*
		 okoSetDM(ptc->wfc[wfc].ctrl, &okodm);
		 
		 */
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;	
}

int drvSetupHardware(control_t *ptc, aomode_t aomode, calmode_t calmode) {
    if (aomode == AO_MODE_CAL) {
        if (calmode == CAL_DARK) {
            logInfo(0, "Configuring hardware for darkfield calibration");
        }
        else if (calmode == CAL_FLAT) {
            logInfo(0, "Configuring hardware for flatfield calibration");
        }
        else if (calmode == CAL_INFL) {
            logInfo(0, "Configuring hardware for influence matrix calibration");
        }
        else if (calmode == CAL_PINHOLE) {
            logInfo(0, "Configuring hardware for subaperture reference calibration");
			// set TT to 5V = middle
			daq2kSetDACs(&daqboard, (int) 32768 + 16384);
        }
        else {
            logWarn("No special setup needed for this calibration mode, ignored");
        }
    }
    else if (aomode == AO_MODE_OPEN || aomode == AO_MODE_CLOSED) {
        logInfo(0, "Configuring hardware for open/closed loop mode calibration");
    }
    else {
        logWarn("No special setup needed for this aomode, ignored");
    }        
    
    return EXIT_SUCCESS;	
}

int MMAvgFramesByte(control_t *ptc, gsl_matrix_float *output, wfs_t *wfs, int rounds) {
	int k, i, j;
	float min, max, sum, tmpvar;
	uint8_t *imgsrc;
    logDebug(0, "Averaging %d frames now (dark, flat, whatever)", rounds);

	gsl_matrix_float_set_zero(output);
	for (k=0; k<rounds; k++) {
		if ((k % (rounds/10)) == 0 && k > 0)
       			logDebug(0 , "Frame %d", k);

//		drvGetImg(&dalsacam, &buffer, NULL, &(wfs->image));
		drvGetImg(ptc, 0);
		imgsrc = (uint8_t *) wfs->image;
		
		for (i=0; i<wfs->res.y; i++) {
			for (j=0; j<wfs->res.x; j++) {
				tmpvar = gsl_matrix_float_get(output, i, j) + (float) imgsrc[i*wfs->res.x +j];
				gsl_matrix_float_set(output, i, j, tmpvar);
			}
		}
	}

	gsl_matrix_float_scale( output, 1/(float) rounds);
	gsl_matrix_float_minmax(output, &min, &max);
	for (j=0; j<wfs->res.x; j++) 
		for (i=0; i<wfs->res.y; i++) 
			sum += gsl_matrix_float_get(output, i, j);
				
        logDebug(0, "Result: min: %.2f, max: %.2f, sum: %.2f, avg: %.2f", \
		min, max, sum, sum/(wfs->res.x * wfs->res.y) );
	
	return EXIT_SUCCESS;
}

// Dark flat calibration, only for subapertures we found previously
int MMDarkFlatSubapByte(wfs_t *wfs, mod_sh_track_t *shtrack) {
	// if correct, CAL_DARKGAIN was run before this is called,
	// and we have wfs->dark and wfs->gain to scale the image with like
	// corrected = ((img*256 - dark) * gain)/256. In ASM (MMX/SSE2)
	// this can be done relatively fast (see ao3.c)
	int sn, i, j, off;
	uint32_t tmp;
	uint8_t *tsrc = (uint8_t *) wfs->image;
	uint16_t *tdark = (uint16_t *) wfs->dark;
	uint16_t *tgain = (uint16_t *) wfs->gain;
	uint8_t *tcorr = (uint8_t *) wfs->corr;

	
	for (sn=0; sn< shtrack->nsubap; sn++) {
		// we loop over each subaperture, and correct only these
		// pixels, because we only measure those anyway, so it's
		// faster
		off = sn * (shtrack->track.x * shtrack->track.y);
		tsrc = ((uint8_t *) wfs->image) + shtrack->subc[sn].y * wfs->res.x + shtrack->subc[sn].x;
		for (i=0; i<shtrack->track.y; i++) {
			for (j=0; j<shtrack->track.x; j++) {
				// we're simulating saturated calculations here
				// with quite some effort. This goes must better
				// in MMX/SSE2.
				// Here we check if src - dark < 0, and we set 
				// the result to zero
				
//				tmp = (( ((uint16_t) tsrc[i*wfs->res.x + j]) << 8) - tdark[off+i*shtrack->track.x + j]); 
				
				//if (tsrc[i*wfs->res.x + j] > 0)
				//logDebug(LOG_NOFORMAT, "s: %u, s8: %u, d: %u, g: %u, t: %u", tsrc[i*wfs->res.x + j], ((uint16_t) tsrc[i*wfs->res.x + j]) << 8, tdark[off+i*shtrack->track.x + j], tgain[off+i*shtrack->track.x + j], tmp);
				
//				if (tmp > (tsrc[i*wfs->res.x + j] << 8))
//					tcorr[off+i*shtrack->track.x + j] = 0;
				// Here we check if we overflow the pixel range
				// after applying a gain, if so we set the pixel
				// to 255
//				else if ((tmp = ((tmp * tgain[off+i*shtrack->track.x + j]) >> 16)) > 255)
//					tcorr[off+i*shtrack->track.x + j] = 255;
				// if none of this happens, we just set the pixel
				// to the value that it should have
//				else
//					tcorr[off+i*shtrack->track.x + j] = tmp;
				
				tcorr[off+i*shtrack->track.x + j] = tsrc[i*wfs->res.x + j];
				//logDebug(LOG_NOFORMAT, "c: %u\n", tcorr[off+i*shtrack->track.x + j]);
			}
		}
	}
//	float srcst[3];
//	float corrst[3];
//	imgGetStats(wfs->corr, DATA_UINT16, NULL, shtrack->nsubap * shtrack->track.x * shtrack->track.y, corrst);
//	imgGetStats(wfs->image, DATA_UINT8, &(wfs->res), -1, srcst);
//	logDebug(LOG_SOMETIMES, "corr: min: %f, max: %f, avg: %f", corrst[0], corrst[1], corrst[2]);
//	logDebug(LOG_SOMETIMES, "src: min: %f, max: %f, avg: %f", srcst[0], srcst[1], srcst[2]);
	//sleep(1);
	
	return EXIT_SUCCESS;
}

// Dark flat calibration
int MMDarkFlatFullByte(wfs_t *wfs, mod_sh_track_t *shtrack) {
	//logDebug(LOG_SOMETIMES, "Slow full-frame darkflat correcting now");
	size_t i, j; // size_t because gsl wants this
	uint8_t* imagesrc = (uint8_t*) wfs->image;
	
	if (wfs->darkim == NULL || wfs->flatim == NULL || wfs->corrim == NULL) {
		logWarn("Dark, flat or correct image memory not available, please calibrate first");
		return EXIT_FAILURE;
	}
	// copy the image to corrim, while doing dark/flat fielding at the same time
//	float max[2], sum[2];
//	sum[0] = sum[1] = 0.0;
//	max[0] = imagesrc[0];
//	max[1] = gsl_matrix_float_get( wfs->darkim, 0, 0);
//	float pix1, pix2;
	for (i=0; (int) i < wfs->res.y; i++) {
		for (j=0; (int) j < wfs->res.x; j++) {
			// pix 1 is flat - dark
			/*
			pix1 = (gsl_matrix_float_get(wfs->flatim, i, j) - \
					gsl_matrix_float_get(wfs->darkim, i, j));
			*/
			// pix 2 is max(raw - dark, 0)
			/*
			pix2 = fmax(imagesrc[i*wfs->res.x +j] - \
						gsl_matrix_float_get(wfs->darkim, i, j), 0);
			*/
			// if flat - dark is 0, we set the output to zero to prevent 
			// dividing by zero, otherwise we take max(pix2 / pix1, 255)
			// we multiply by 128 because (raw-dark)/(flat-dark) is
			// 1 on average (I guess), multiply by static 128 to get an image
			// at all. Actually this should be average(flat-dark), but that's
			// too expensive here, this should work fine :P
//			if (pix1 <= 0)
//				gsl_matrix_float_set(wfs->corrim, i, j, 0.0);
//			else 
//				gsl_matrix_float_set(wfs->corrim, i, j, \
//									 fmin(128 * pix2 / pix1, 255));
			
			
			gsl_matrix_float_set(wfs->corrim, i, j, \
								 imagesrc[i*wfs->res.x +j]);
			
			//				 fmaxf((((float) imagesrc[i*wfs->res.x +j]) - \
			gsl_matrix_float_get(wfs->darkim, i, j)), 0)/ \
			(gsl_matrix_float_get(wfs->flatim, i, j) -\
			gsl_matrix_float_get(wfs->darkim, i, j)));
			//sum[0] += imagesrc[i*wfs->res.x +j];
			//			sum[1] += gsl_matrix_float_get(wfs->darkim, i, j);
			//sum[1] += gsl_matrix_float_get(wfs->corrim, i, j);
			//if ( imagesrc[i*wfs->res.x +j] > max[0])
			//max[0] = imagesrc[i*wfs->res.x +j];
			//			if ( gsl_matrix_float_get(wfs->darkim, i, j)>max[1])
			//				max[1] =  gsl_matrix_float_get(wfs->darkim, i, j);
			//if ( gsl_matrix_float_get(wfs->corrim, i, j)>max[1])
			//max[1] =  gsl_matrix_float_get(wfs->corrim, i, j);
			//								 (gsl_matrix_float_get(wfs->flatim, i, j) - \
			//								  gsl_matrix_float_get(wfs->darkim, i, j)));
		}
	}
//	float corrstats[3];
//	float srcstats[3];
//	imgGetStats(imagesrc, DATA_UINT8, &(wfs->res), -1, srcstats);
//	imgGetStats(wfs->corrim, DATA_GSL_M_F, &(wfs->res), -1, corrstats);
//	
//	logDebug(LOG_SOMETIMES, "src: min %f, max %f, avg %f", srcstats[0], srcstats[1], srcstats[2]);
//	logDebug(LOG_SOMETIMES, "corr: min %f, max %f, avg %f", corrstats[0], corrstats[1], corrstats[2]);
	//logDebug(LOG_SOMETIMES, "src: max %f, avg %f", max[0], sum[0]/(wfs->res.x * wfs->res.y));
	//logDebug(LOG_SOMETIMES, "corr: max %f,avg %f", max[1], sum[1]/(wfs->res.x * wfs->res.y));
		
	return EXIT_SUCCESS;
}
