/*! 
	@file foam_primemod-mcmath.c
	@author @authortim
	@date isoD	2008-04-18

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

// Shack Hartmann tracking info
mod_sh_track_t shtrack;

int modInitModule(control_t *ptc, config_t *cs_config) {
	logInfo(0, "This is the McMath prime module, enjoy.");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant initialliy
	ptc->wfs_count = 1;					// 2 FW, 1 WFS and 2 WFC
	ptc->wfc_count = 2;
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
	ptc->wfs[0].darkfile = "mcmath_dark.gsldump";	
	ptc->wfs[0].flatfile = "mcmath_flat.gsldump";
	ptc->wfs[0].skyfile = "mcmath_sky.gsldump";
	ptc->wfs[0].scandir = AO_AXES_XY;
	
	// configure WFC 0
	ptc->wfc[0].name = "Okotech DM";
	ptc->wfc[0].nact = 37;
	ptc->wfc[0].gain = 1.0;
	ptc->wfc[0].type = WFC_DM;
	
	// configure WFC 1
	ptc->wfc[1].name = "TT";
	ptc->wfc[1].nact = 2;
	ptc->wfc[1].gain = 1.0;
	ptc->wfc[1].type = WFC_TT;
	
	// configure filter 0
	ptc->filter[0].name = "Telescope FW";
	ptc->filter[0].nfilts = 3;
	ptc->filter[0].filters[0] = FILT_PINHOLE;
	ptc->filter[0].filters[1] = FILT_OPEN;
	ptc->filter[0].filters[2] = FILT_CLOSED;

	ptc->filter[0].name = "WFS FW";
	ptc->filter[0].nfilts = 3;
	ptc->filter[0].filters[0] = FILT_PINHOLE;
	ptc->filter[0].filters[1] = FILT_OPEN;
	ptc->filter[0].filters[2] = FILT_CLOSED;
	
	// configure ITIFG camera
	
	dalsacam.module = 48;
	dalsacam.device_name = "/dev/ic0dma";
	dalsacam.config_file = "../config/dalsa-cad6-pcd.cam";
	
	buffer.frames = 8;
	
	// configure the daqboard
	
	daqboard.device = "daqBoard2k0";	// we use the first daqboard
	daqboard.nchans = 4;				// we use 4 analog chans [-10, 10] V
	daqboard.minvolt = -10.0;
	daqboard.maxvolt = 10.0;
	daqboard.iop2conf[0] = 0;
	daqboard.iop2conf[1] = 0;
	daqboard.iop2conf[2] = 1;
	daqboard.iop2conf[3] = 1;			// use digital IO ports for {out, out, in, in}
	
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
	
	// shtrack configuring
    // we have a CCD of WxH, with a lenslet array of WlxHl, such that
    // each lenslet occupies W/Wl x H/Hl pixels, and we use track.x x track.y
    // pixels to track the CoG or do correlation tracking.
	shtrack.cells.x = 8;				// we're using a 8x8 lenslet array
	shtrack.cells.y = 8;
	shtrack.shsize.x = ptc->wfs[0].res.x/shtrack.cells.x;
	shtrack.shsize.y = ptc->wfs[0].res.y/shtrack.cells.y;
	shtrack.track.x = shtrack.shsize.x/2;   // tracker windows are half the size of the lenslet grid things
    shtrack.track.y = shtrack.shsize.y/2;
	shtrack.pinhole = "mcmath_pinhole.gsldump";
	shtrack.influence = "mcmath_influence.gsldump";
	
	// configure cs_config here
	cs_config->listenip = "0.0.0.0";	// listen on any IP by defaul
	cs_config->listenport = 10000;		// listen on port 10000 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam-mm";	// prepend logging with 'foam'
	cs_config->use_stdout = true;		// do use stdout
	cs_config->loglevel = LOGDEBUG;		// log error, info and debug
	cs_config->logfrac = 100;			// log verbose messages only every 100 frames
	cs_config->infofile = NULL;			// don't log anything to file
	cs_config->errfile = NULL;
	cs_config->debugfile = NULL;

#ifdef FOAM_MCMATH_DISPLAY
	// init display
	disp.caption = "McMath - WFS";
	disp.res.x = ptc->wfs[0].res.x;
	disp.res.y = ptc->wfs[0].res.y;
	disp.flags = SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_RESIZABLE;
    disp.autocontrast = 1;
	
	modInitDraw(&disp);
#endif
	
	drvInitBoard(&dalsacam);
	drvInitBufs(&buffer, &dalsacam);
	
	return EXIT_SUCCESS;
}

void modStopModule(control_t *ptc) {
#ifdef FOAM_MCMATH_DISPLAY
	modFinishDraw(disp.screen);
#endif
	
	drvStopGrab(&dalsacam);
	drvStopBufs(&buffer, &dalsacam);
	drvStopBoard(&dalsacam);
}

// OPEN LOOP ROUTINES //
/*********************/

int modOpenInit(control_t *ptc) {
	// start grabbing frames
	return drvInitGrab(&dalsacam);
}

int modOpenLoop(control_t *ptc) {
	// get an image, without using a timeout
	drvGetImg(&dalsacam, &buffer, NULL);

//	MMDarkFlatCorrByte(&(ptc->wfs[0]));
	
#ifdef FOAM_MCMATH_DISPLAY
	modDrawStuff(ptc, 0, disp.screen, &shtrack);
#endif
	return EXIT_SUCCESS;
}

int modOpenFinish(control_t *ptc) {
	// stop grabbing frames
	drvStopGrab(&dalsacam);
	return EXIT_SUCCESS;
}

// CLOSED LOOP ROUTINES //
/************************/

int modClosedInit(control_t *ptc) {
	// start grabbing frames
	return drvInitGrab(&dalsacam);
}

int modClosedLoop(control_t *ptc) {
	// get an image, without using a timeout
	drvGetImg(&dalsacam, &buffer, NULL);
	
	
	return EXIT_SUCCESS;
}

int modClosedFinish(control_t *ptc) {
	// stop grabbing frames
	drvStopGrab(&dalsacam);

	return EXIT_SUCCESS;
}

// MISC ROUTINES //
/*****************/

int modCalibrate(control_t *ptc) {
	if (ptc->calmode == CAL_DARK) {
		// take 100 dark frames, and average
		MMAvgFramesByte(ptc->wfs[0].darkim, &(ptc->wfs[0]), 100);
	}
	else if (ptc->calmode == CAL_FLAT) {
		// take 100 flat frames, and average
		MMAvgFramesByte(ptc->wfs[0].flatim, &(ptc->wfs[0]), 100);
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
	int tmpint;
	float tmpfloat;
	
 	if (strcmp(list[0],"help") == 0) {
		// give module specific help here
		if (count > 1) { 

			if (strcmp(list[1], "display") == 0) {
#ifdef FOAM_MCMATH_DISPLAY
				tellClient(client->buf_ev, "\
200 OK HELP DISPLAY\n\
display <raw|calib>:    display raw ccd output or calibrated image with meta-info.\n\
resetdm [voltage]:      reset the DM to a certain voltage for all acts. default=0\n\
resetdaq [voltage]:     reset the DAQ analog outputs to a certain voltage. default=0\n\
");
#endif
			}
			else // we don't know. tell this to parseCmd by returning 0
				return 0;
		}
		else {
			tellClient(client->buf_ev, "This is the dummy module and does not provide any additional commands");
		}
	}
#ifdef FOAM_MCMATH_DISPLAY
 	else if (strcmp(list[0], "display") == 0) {
		// give module specific help here
		if (count > 1) {
			if (strcmp(list[1], "raw") == 0) {
				tellClient(client->buf_ev, "200 OK DISPLAY RAW");
			}
			else if (strcmp(list[1], "calib") == 0) {
				tellClient(client->buf_ev, "200 OK DISPLAY CALIB");
			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN DISPLAY");
				// we don't know. tell this to parseCmd by returning 0
				return 0;
			}
		}
		else {
			tellClient(client->buf_ev, "402 DISPLAY REQUIRES ARGS");
		}
	}
#endif
 	else if (strcmp(list[0], "resetdm") == 0) {
		// give module specific help here
		if (count > 1) {
			tmpint = strtol(list[1], NULL, 10);
			
			if (tmpint >= okodm.minvolt && tmpint <= okodm.maxvolt) {
				if (drvSetAllOkoDM(&okodm, tmpint) == EXIT_SUCCESS)
					tellClients("200 OK RESETDM %dV", tmpint);
				else
					tellClient(client->buf_ev, "300 ERROR RESETTING DM");
					
			}
			else {
				tellClient(client->buf_ev, "403 INCORRECT VOLTAGE!");
				return 0;
			}
		}
		else {
			if (drvRstOkoDM(&okodm) == EXIT_SUCCESS)
				tellClients("200 OK RESETDM 0V");
			else 
				tellClient(client->buf_ev, "300 ERROR RESETTING DM");
			
		}
	}
 	else if (strcmp(list[0], "resetdaq") == 0) {
		// give module specific help here
		if (count > 1) {
			tmpfloat = strtof(list[1], NULL);
			
			if (tmpfloat >= daqboard.minvolt && tmpfloat <= daqboard.maxvolt) {
				drvDaqSetDACs(&daqboard, (int) 65536*(tmpfloat-daqboard.minvolt)/(daqboard.maxvolt-daqboard.minvolt));
				tellClients("200 OK RESETDAQ %fV", tmpfloat);
			}
			else {
				tellClient(client->buf_ev, "403 INCORRECT VOLTAGE!");
				return 0;
			}
		}
		else {
			drvDaqSetDACs(&daqboard, 65536*(-daqboard.minvolt)/(daqboard.maxvolt-daqboard.minvolt));
			tellClients("200 OK RESETDAQ 0.0V");			
		}
	}
 	else if (strcmp(list[0], "calibrate") == 0) {
		if (count > 1) {
			if (strcmp(list[1], "dark") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_DARK;
				// add message to the users
			}
			else if (strcmp(list[1], "flat") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_FLAT;
				// add message to the users

			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN CALIBRATION");
				return 0;
			}
		}
		else {
			tellClient(client->buf_ev, "402 CALIBRATE REQUIRES ARGS");
			return 0;
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

int drvSetActuator(control_t *ptc, int wfc) {
	if (wfc == 0) {			// Okotech DM
		// use okodm routines here
	}
	else if (wfc == 1) {	// Tip-tilt mirror
		// use daq routines here
	}
	
	return EXIT_SUCCESS;
}

int drvFilterWheel(control_t *ptc, fwheel_t filter) {
	if (filter == FILT_PINHOLE) {
        logInfo(0, "Setting filter to pinhole '%d'", filter);
	}
	else if (filter == FILT_DARK || filter == FILT_CLOSED) {
        logInfo(0, "Setting filter to closed/darkfield '%d'", filter);
	}
	else if (filter == FILT_OPEN || filter == FILT_FLAT) {
        logInfo(0, "Setting filter to open/flatfield '%d'", filter);
	}
	else {
		logWarn("Unknown filter '%d'", filter);
	}
	
	return EXIT_SUCCESS;
}

int MMAvgFramesByte(gsl_matrix_float *output, wfs_t *wfs, int rounds) {
	int k, i, j;
	uint8_t* imagesrc = (uint8_t *) wfs->image;
	
	for (k=0; k<rounds; k++) {
		drvGetImg(&dalsacam, &buffer, NULL);
		
		for (j=0; j<wfs->res.x; j++) {
			for (i=0; i<wfs->res.y; i++) {
				gsl_matrix_float_set(output, i, j, \
					 (float) gsl_matrix_float_get(output, i, j) + \
					 (uint8_t) imagesrc[j*wfs->res.x +i]);
			}
		}
		gsl_matrix_float_scale( output, 1/(float) rounds);
	}
	
	return EXIT_SUCCESS;
}

// Dark flat calibration
int MMDarkFlatCorrByte(wfs_t *wfs) {
	size_t i, j; // size_t because gsl wants this
	uint8_t* imagesrc = (uint8_t*) wfs->image;
	
	if (wfs->darkim != NULL && wfs->flatim != NULL) {
		// copy raw image to corrected image memory first
		for (j=0; j < wfs->res.x; j++) {
			for (i=0; i < wfs->res.y; i++) {
				// !!!:tim:20080505 double casting a little weird?
				gsl_matrix_float_set(wfs->corrim, i, j, (float) imagesrc[j*wfs->res.x +i]);
			}
		}
		
		// now do raw-dark/(flat-dark), flat-dark is already stored in wfs->flatim
		gsl_matrix_float_sub (wfs->corrim, wfs->darkim);
		gsl_matrix_float_div_elements (wfs->corrim, wfs->flatim);
	}
	return EXIT_SUCCESS;
}

