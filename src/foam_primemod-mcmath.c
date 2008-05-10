/*! 
 @file foam_primemod-mcmath.c
 @author @authortim
 @date 2008-04-18
 
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

#ifndef FOAM_MODULES_DISLAY_SHSUPPORT
#define FOAM_MODULES_DISLAY_SHSUPPORT
#endif

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
	logInfo(0, "This is the McMath-Pierce prime module, enjoy.");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant initialliy
	ptc->logfrac = 100;                 // log verbose messages only every 100 frames
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
    	ptc->wfs[0].id = 0;
	ptc->wfs[0].fieldframes = 1000;     // take 1000 frames for a dark or flatfield
	
	// configure WFC 0
	ptc->wfc[0].name = "Okotech DM";
	ptc->wfc[0].nact = 37;
	ptc->wfc[0].gain.p = 1.0;
	ptc->wfc[0].gain.i = 1.0;
	ptc->wfc[0].gain.d = 1.0;
	ptc->wfc[0].type = WFC_DM;
    ptc->wfc[0].id = 0;
	
	// configure WFC 1
	ptc->wfc[1].name = "TT";
	ptc->wfc[1].nact = 2;
	ptc->wfc[1].gain.p = 1.0;
	ptc->wfc[1].gain.i = 1.0;
	ptc->wfc[1].gain.d = 1.0;
	ptc->wfc[1].type = WFC_TT;
    ptc->wfc[1].id = 1;
	
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
	cs_config->syslog_prepend = "foam-mm";	// prepend logging with 'foam-mm'
	cs_config->use_stdout = true;		// do use stdout
	cs_config->loglevel = LOGDEBUG;		// log error, info and debug
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
    disp.brightness = 0;
    disp.contrast = 5;
    disp.dispsrc = DISPSRC_RAW;         // use the raw ccd output
    disp.dispover = DISPOVERLAY_GRID;   // display the SH grid
	
	modInitDraw(&disp);
#endif
	
	drvInitBoard(&dalsacam);
	drvInitBufs(&buffer, &dalsacam);
	
	// update the pointer to the wfs image
	ptc->wfs[0].image = buffer.data;
	
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
	if (drvGetImg(&dalsacam, &buffer, NULL, &(ptc->wfs->image)) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	
	//	MMDarkFlatCorrByte(&(ptc->wfs[0]));
	
#ifdef FOAM_MCMATH_DISPLAY
    if (ptc->frames % ptc->logfrac == 0) {
		modDrawStuff((&ptc->wfs[0]), &disp, &shtrack);
	    logInfo(0, "Current framerate: %.2f FPS", ptc->fps);
    }
#endif
	return EXIT_SUCCESS;
}

int modOpenFinish(control_t *ptc) {
	// stop grabbing frames
	return drvStopGrab(&dalsacam);
}

// CLOSED LOOP ROUTINES //
/************************/

int modClosedInit(control_t *ptc) {
	// start grabbing frames
	return drvInitGrab(&dalsacam);
}

int modClosedLoop(control_t *ptc) {
	// get an image, without using a timeout
	drvGetImg(&dalsacam, &buffer, NULL, &(ptc->wfs->image));
	// update the pointer to the wfs image
	ptc->wfs[0].image = buffer.data;
	
	return EXIT_SUCCESS;
}

int modClosedFinish(control_t *ptc) {
	// stop grabbing frames
	return drvStopGrab(&dalsacam);
}

// MISC ROUTINES //
/*****************/

int modCalibrate(control_t *ptc) {
	FILE *fieldfd;
	wfs_t *wfsinfo = &(ptc->wfs[0]);
	if (ptc->calmode == CAL_DARK) {
		// take dark frames, and average
		logInfo(0, "Starting darkfield calibration now");
		if (drvInitGrab(&dalsacam) != EXIT_SUCCESS) 
			return EXIT_FAILURE;

		// check if memory is allocated yet
		if (wfsinfo->darkim == NULL) {
			wfsinfo->darkim = gsl_matrix_float_calloc(wfsinfo->res.x, wfsinfo->res.y);
		}

		MMAvgFramesByte(wfsinfo->darkim, &(ptc->wfs[0]), wfsinfo->fieldframes);
		if (drvStopGrab(&dalsacam) != EXIT_SUCCESS)
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
	}
	else if (ptc->calmode == CAL_FLAT) {
		logInfo(0, "Starting flatfield calibration now");
		// take flat frames, and average
		if (drvInitGrab(&dalsacam) != EXIT_SUCCESS) 
			return EXIT_FAILURE;
		// check if memory is allocated yet
		if (wfsinfo->flatim == NULL) {
			wfsinfo->flatim = gsl_matrix_float_calloc(wfsinfo->res.x, wfsinfo->res.y);
		}
		MMAvgFramesByte(wfsinfo->flatim, &(ptc->wfs[0]), wfsinfo->fieldframes);
		if (drvStopGrab(&dalsacam) != EXIT_SUCCESS)
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
	}
	else if (ptc->calmode == CAL_SUBAPSEL) {
		logInfo(0, "Starting subaperture selection now");
		//modOpenInit(ptc);
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
				tellClient(client->buf_ev, "\
200 OK HELP DISPLAY\n\
display <source>:       change the display source.\n\
   raw:                 direct images from the camera.\n\
   calib:               dark/flat corrected images.\n\
   dark:                show the darkfield being used.\n\
   flat:                show the flatfield being used.\n\
");
			}
			else if (strcmp(list[1], "vid") == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP VID\n\
vid <mode> [val]:       configure the video output.\n\
   auto:                use auto contrast/brightness.\n\
   c [int]:             use manual c/b with this contrast.\n\
   c [int]:             use manual c/b with this brightness.\n\
");
			}
			else if (strcmp(list[1], "set") == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP SET\n\
set [prop] [val]:       set or query property values.\n\
   lf [int]:            set the logfraction.\n\
   ff [int]:            set the number of frames to use for dark/flats.\n\
   -:                   if no prop is given, query the values.\
");
			}
			else if (strcmp(list[1], "calibrate") == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP CALIBRATE\n\
calibrate <mode>:       calibrate the ao system.\n\
   dark:                take a darkfield by averaging %d frames.\n\
   flat:                take a flatfield by averaging %d frames.\n\
   subapsel:            select some subapertures.\n\
", ptc->wfs[0].fieldframes, ptc->wfs[0].fieldframes);
			}
			else // we don't know. tell this to parseCmd by returning 0
				return 0;
		}
		else {
			tellClient(client->buf_ev, "\
=== prime module options ===\n\
display <source>:       tell foam what display source to use.\n\
vid <auto|c|v> [int]:   use autocontrast/brightness, or set manually.\n\
resetdm [voltage]:      reset the DM to a certain voltage for all acts. def=0\n\
resetdaq [voltage]:     reset the DAQ analog outputs to a certain voltage. def=0\n\
set [prop]:             set or query certain properties.\n\
calibrate <mode>:       calibrate the ao system (dark, flat, subapt, etc).\
");
		}
	}
#ifdef FOAM_MCMATH_DISPLAY
 	else if (strcmp(list[0], "display") == 0) {
		if (count > 1) {
			if (strcmp(list[1], "raw") == 0) {
				tellClient(client->buf_ev, "200 OK DISPLAY RAW");
                disp.dispsrc = DISPSRC_RAW;
                // example how to remove mask 'DISPMODE_RAW' from dispmode
                // raw mode:  00001
                // !raw mode: 11110
                // old mode:  01101
                // !raw&old:  01100 
			}
			else if (strcmp(list[1], "calib") == 0) {
                disp.dispsrc = DISPSRC_CALIB;
				tellClient(client->buf_ev, "200 OK DISPLAY CALIB");
			}
			else if (strcmp(list[1], "dark") == 0) {
				if  (ptc->wfs[0].darkim == NULL) {
					tellClient(client->buf_ev, "400 ERROR DARKFIELD NOT AVAILABLE");
				}
				else {
					disp.dispsrc = DISPSRC_DARK;
					tellClient(client->buf_ev, "200 OK DISPLAY DARK");
				}
			}
			else if (strcmp(list[1], "flat") == 0) {
				if  (ptc->wfs[0].flatim == NULL) {
					tellClient(client->buf_ev, "400 ERROR FLATFIELD NOT AVAILABLE");
				}
				else {
					disp.dispsrc = DISPSRC_FLAT;
					tellClient(client->buf_ev, "200 OK DISPLAY FLAT");
				}
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
 	else if (strcmp(list[0], "set") == 0) {
		if (count > 2) {
			tmpint = strtol(list[2], NULL, 10);
			tmpfloat = strtof(list[2], NULL);
			if (strcmp(list[1], "lf") == 0) {
				ptc->logfrac = tmpint;
				tellClient(client->buf_ev, "200 OK SET LOGFRAC TO %d", tmpint);
				return 1;
			}
			else if (strcmp(list[1], "ff") == 0) {
				ptc->wfs[0].fieldframes = tmpint;
				tellClient(client->buf_ev, "200 OK SET FIELDFRAMES TO %d", tmpint);
			}
			else {
				tellClient(client->buf_ev, "401 UNKNOWN PROPERTY, CANNOT SET");
				return 0;
			}
		}
		else {
			tellClient(client->buf_ev, "200 OK VALUES AS FOLLOWS:\n\
logfrac (lf):           %d\n\
fieldframes (ff):       %d\n\
SH array:               %dx%d\n\
cell size:              %dx%d\n\
ccd size:               %dx%d\n\
", ptc->logfrac, ptc->wfs[0].fieldframes, shtrack.cells.x, shtrack.cells.y,\
shtrack.shsize.x, shtrack.shsize.y, ptc->wfs[0].res.x, ptc->wfs[0].res.y);
		}
	}
 	else if (strcmp(list[0], "vid") == 0) {
		if (count > 1) {
			if (strcmp(list[1], "auto") == 0) {
				disp.autocontrast = 1;
				tellClient(client->buf_ev, "200 OK USING AUTO SCALING");
			}
			else if (strcmp(list[1], "c") == 0) {
				if (count > 2) {
					tmpint = strtol(list[2], NULL, 10);
					disp.autocontrast = 0;
					disp.contrast = tmpint;
					tellClient(client->buf_ev, "200 OK CONTRAST %d", tmpint);
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
					tellClient(client->buf_ev, "200 OK BRIGHTNESS %d", tmpint);
				}
				else {
					tellClient(client->buf_ev, "402 NO BRIGHTNESS GIVEN");
				}
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
 	else if (strcmp(list[0], "calibrate") == 0) {
		if (count > 1) {
			if (strcmp(list[1], "dark") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_DARK;
                tellClient(client->buf_ev, "200 OK DARKFIELDING NOW");
		pthread_cond_signal(&mode_cond);
				// add message to the users
			}
			else if (strcmp(list[1], "flat") == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_FLAT;
                tellClient(client->buf_ev, "200 OK FLATFIELDING NOW");
		pthread_cond_signal(&mode_cond);
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

int drvSetActuator(wfc_t *wfc) {
	if (wfc->type == 0) {			// Okotech DM
		// use okodm routines here
	}
	else if (wfc == 1) {	// Tip-tilt mirror
		// use daq routines here
	}
	
	return EXIT_SUCCESS;
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

int MMAvgFramesByte(gsl_matrix_float *output, wfs_t *wfs, int rounds) {
	int k, i, j;
	float min, max, sum, tmpvar;
	uint8_t *imgsrc;
    logDebug(0, "Averaging %d frames now (dark, flat, whatever)", rounds);

	gsl_matrix_float_set_zero(output);
	for (k=0; k<rounds; k++) {
		if ((k % (rounds/10)) == 0 && k > 0)
       			logDebug(0 , "Frame %d", k);

		drvGetImg(&dalsacam, &buffer, NULL, &(wfs->image));
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

