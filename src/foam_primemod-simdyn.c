/*! 
 @file foam_primemod-simdyn.c
 @author @authortim
 @date 2008-07-03
 
 @brief This is a dynamical simulation mode, which simulates an AO system at wavefront level 
 
 This primemodule can be used to simulate a complete AO setup, starting with a perturbed wavefront
 and following this through the complete optical setup.
 */

// HEADERS //
/***********/

// We need these for modMessage, see foam_cs.c
extern pthread_mutex_t mode_mutex;
extern pthread_cond_t mode_cond;


// GLOBALS //
/***********/

#define FOAM_CONFIG_PRE "simdyn"

// Displaying
#ifdef FOAM_SIMDYN_DISPLAY
mod_display_t disp;
#endif

// Shack Hartmann tracking info
mod_sh_track_t shtrack;
// Simulation parameters
mod_sim_t simparams;

int modInitModule(control_t *ptc, config_t *cs_config) {
	logInfo(0, "This is the simdyn prime module, enjoy.");
	
	// populate ptc here
	ptc->mode = AO_MODE_LISTEN;			// start in listen mode (safe bet, you probably want this)
	ptc->calmode = CAL_INFL;			// this is not really relevant initialliy
	ptc->logfrac = 10;                  // log verbose messages only every 100 frames
	ptc->wfs_count = 1;					// just 1 static 'wfs' for simulation
	ptc->wfc_count = 1;
	ptc->fw_count = 0;
	
	// allocate memory for filters, wfc's and wfs's
	// no filter, no WFS in simulation
//	ptc->filter = (filtwheel_t *) calloc(ptc->fw_count, sizeof(filtwheel_t));
	ptc->wfc = (wfc_t *) calloc(ptc->wfc_count, sizeof(wfc_t));
	ptc->wfs = (wfs_t *) calloc(ptc->wfs_count, sizeof(wfs_t));
	
	// set image to something static
//	coord_t imgres;
//	uint8_t *imgptr = (uint8_t *) ptc->wfs[0].image;
//	modReadIMGArrByte("../config/simstatic-irr.pgm", &(imgptr), &imgres);
//	ptc->wfs[0].image  = (void *) imgptr;
	
	// configure WFS 0
	ptc->wfs[0].name = "SH WFS - static";
	ptc->wfs[0].res.x = 256;
	ptc->wfs[0].res.y = 256;
	ptc->wfs[0].bpp = 8;
	// this is where we will look for dark/flat/sky images
	ptc->wfs[0].darkfile = FOAM_CONFIG_PRE "_dark.gsldump";	
	ptc->wfs[0].flatfile = FOAM_CONFIG_PRE "_flat.gsldump";
	ptc->wfs[0].skyfile = FOAM_CONFIG_PRE "_sky.gsldump";
	ptc->wfs[0].scandir = AO_AXES_XY;
	ptc->wfs[0].id = 0;
	ptc->wfs[0].fieldframes = 1000;     // take 1000 frames for a dark or flatfield
		
	// Simulation configuration
	simparams.wind.x = 5;
	simparams.wind.y = 0;
	simparams.seeingfac = 0.2;
	// these files are needed for AO simulation and will be read in by simInit()
	simparams.wf = "../config/wavefront.png";
	simparams.apert = "../config/apert15-256.pgm";
	simparams.actpat = "../config/dm37-256.pgm";
	// resolution of the simulated image
	simparams.currimgres.x = ptc->wfs[0].res.x;
	simparams.currimgres.y = ptc->wfs[0].res.y;
	// These need to be init to NULL
	simparams.shin = simparams.shout = simparams.plan_forward = NULL;
	simparams.wisdomfile = FOAM_CONFIG_PRE "_fftw-wisdom";
	if(simInit(&simparams) != EXIT_SUCCESS)
		logErr("Failed to initialize simulation module.");

	ptc->wfs[0].image = (void *) simparams.currimg;


	// shtrack configuring
    // we have an image of WxH, with a lenslet array of WlxHl, such that
    // each lenslet occupies W/Wl x H/Hl pixels, and we use track.x x track.y
    // pixels to track the CoG or do correlation tracking.
	shtrack.cells.x = 8;				// we're using a 16x16 lenslet array (fake)
	shtrack.cells.y = 8;
	shtrack.shsize.x = ptc->wfs[0].res.x/shtrack.cells.x;
	shtrack.shsize.y = ptc->wfs[0].res.y/shtrack.cells.y;
	shtrack.track.x = shtrack.shsize.x/2;   // tracker windows are half the size of the lenslet grid things
	shtrack.track.y = shtrack.shsize.y/2;
	shtrack.pinhole = FOAM_CONFIG_PRE "_pinhole.gsldump";
	shtrack.influence = FOAM_CONFIG_PRE "_influence.gsldump";
	shtrack.samxr = -1;			// 1 row edge erosion
	shtrack.samini = 30;			// minimum intensity for subaptselection 10
	// init the shtrack module now
	if (modInitSH(&(ptc->wfs[0]), &shtrack) != EXIT_SUCCESS)
		logErr("Failed to initialize shack-hartmann module.");
	
	// configure cs_config here
	cs_config->listenip = "0.0.0.0";	// listen on any IP by defaul
	cs_config->listenport = 10000;		// listen on port 10000 by default
	cs_config->use_syslog = false;		// don't use the syslog
	cs_config->syslog_prepend = "foam-stat";	// prepend logging with 'foam-stat'
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
#ifdef FOAM_SIMDYN_DISPLAY
	// init display
	disp.caption = "WFS #1";
	disp.res.x = ptc->wfs[0].res.x;
	disp.res.y = ptc->wfs[0].res.y;
	disp.autocontrast = 0;
	disp.brightness = 0;
	disp.contrast = 1;
	disp.dispsrc = DISPSRC_RAW;         // use the raw ccd output
	disp.dispover = DISPOVERLAY_GRID;   // display the SH grid
	disp.col.r = 255;
	disp.col.g = 255;
	disp.col.b = 255;
	
	displayInit(&disp);
#endif
	
#ifdef __APPLE__
	// We need this to fix autoreleasepool errors, I think
	// see https://savannah.gnu.org/bugs/?20957
	// and http://www.idevgames.com/forum/archive/index.php/t-7710.html
	// call this at the beginning of the thread:
	// update: This is objective C and does not work out of the box...
	//NSAutoreleasePool	 *autoreleasepool = [[NSAutoreleasePool alloc] init];
#endif
	
	return EXIT_SUCCESS;
}

void modStopModule(control_t *ptc) {
#ifdef FOAM_SIMDYN_DISPLAY
	displayFinish(&disp);
#endif
	
#ifdef __APPLE__
	// need to call this before the thread dies:
	// update: This is objective C and does not work out of the box...	
//	[autoreleasepool release];
	// but there is a problem as this is not always the end of the thread
	// it IS for the two-threaded model (1 networking, 1 ao) however,
	// so if compiling on OS X, we can only use two threads I'm afraid...
#endif
}

// OPEN LOOP ROUTINES //
/*********************/

int modOpenInit(control_t *ptc) {
	// set disp source to full calib
	disp.dispsrc = DISPSRC_FULLCALIB;

	// nothing to init for static simulation
	return EXIT_SUCCESS;
}

int modOpenLoop(control_t *ptc) {
	static char title[64];
	
	// Get simulated image for the first WFS
	drvGetImg(ptc, 0);
	
	// dark-flat the whole frame
	MMDarkFlatFullByte(&(ptc->wfs[0]), &shtrack);
	
//	modCogTrack(ptc->wfs[0].corrim, DATA_GSL_M_F, ALIGN_RECT, &shtrack, NULL, NULL);
	
#ifdef FOAM_SIMDYN_DISPLAY
    if (ptc->frames % ptc->logfrac == 0) {
		displayDraw((&ptc->wfs[0]), &disp, &shtrack);
		displaySDLEvents(&disp);
		logInfo(0, "Current framerate: %.2f FPS", ptc->fps);
		snprintf(title, 64, "%s (O) %.2f FPS", disp.caption, ptc->fps);
		SDL_WM_SetCaption(title, 0);
    }
#endif
	usleep(100000);
	return EXIT_SUCCESS;
}

int modOpenFinish(control_t *ptc) {
	// stop
	return EXIT_SUCCESS;
}

// CLOSED LOOP ROUTINES //
/************************/

int modClosedInit(control_t *ptc) {
	// set disp source to calib
	disp.dispsrc = DISPSRC_FASTCALIB;		
	// start
	return EXIT_SUCCESS;
}

int modClosedLoop(control_t *ptc) {
	static char title[64];
	int sn;
	
	// Get simulated image for the first WFS
	drvGetImg(ptc, 0);
	
	// dark-flat the whole frame
	MMDarkFlatSubapByte(&(ptc->wfs[0]), &shtrack);

	// try to get the center of gravity 
	//modCogTrack(ptc->wfs[0].corrim, DATA_GSL_M_F, ALIGN_RECT, &shtrack, NULL, NULL);
	modCogTrack(ptc->wfs[0].corr, DATA_UINT8, ALIGN_SUBAP, &shtrack, NULL, NULL);
	
#ifdef FOAM_SIMDYN_DISPLAY
    if (ptc->frames % ptc->logfrac == 0) {
		displayDraw((&ptc->wfs[0]), &disp, &shtrack);
		displaySDLEvents(&disp);
		logInfo(0, "Current framerate: %.2f FPS", ptc->fps);
		logInfo(0, "Displacements per subapt in (x, y) pairs (%d subaps):", shtrack.nsubap);
		for (sn = 0; sn < shtrack.nsubap; sn++)
			logInfo(LOG_NOFORMAT, "(%.1f,%.1f)", \
			gsl_vector_float_get(shtrack.disp, 2*sn + 0), \
			gsl_vector_float_get(shtrack.disp, 2*sn + 1));

		logInfo(LOG_NOFORMAT, "\n");
		snprintf(title, 64, "%s (C) %.2f FPS", disp.caption, ptc->fps);
		SDL_WM_SetCaption(title, 0);
    }
#endif
	usleep(100000);
	return EXIT_SUCCESS;
}

int modClosedFinish(control_t *ptc) {
	// stop
	return EXIT_SUCCESS;
}

// MISC ROUTINES //
/*****************/

int modCalibrate(control_t *ptc) {
	FILE *fieldfd;		// to open some files (dark, flat, ...)
	char title[64]; 	// for the window title
	int i, j, sn;
	float min, max, sum, pix;			// some fielding stats
	wfs_t *wfsinfo = &(ptc->wfs[0]);	// shortcut
	dispsrc_t oldsrc = disp.dispsrc;	// store the old display source here since we might just have to show dark or flatfields
	int oldover = disp.dispover;		// store the old overlay here

	if (ptc->calmode == CAL_DARK) {
		// take dark frames, and average
		logInfo(0, "Starting darkfield calibration now");

		// we fake a darkfield here (random pixels between 2-6)
		min = max = 4.0;
		sum = 0.0;
		for (i=0; i<wfsinfo->res.y; i++) {
			 for (j=0; j<wfsinfo->res.x; j++) {
				 pix = (drand48()*4.0)+2.0;
				 gsl_matrix_float_set(wfsinfo->darkim, i, j, pix);
				 if (pix > max) max = pix;
				 else if (pix < min) min = pix;
				 sum += pix;
			 }
		}

		// saving image for later usage
		fieldfd = fopen(wfsinfo->darkfile, "w+");	
		if (!fieldfd)  {
			logWarn("Could not open darkfield storage file '%s', not saving darkfield (%s).", wfsinfo->darkfile, strerror(errno));
			return EXIT_SUCCESS;
		}
		gsl_matrix_float_fprintf(fieldfd, wfsinfo->darkim, "%.10f");
		fclose(fieldfd);
		logInfo(0, "Darkfield calibration done (m: %f, m: %f, s: %f, a: %f), and stored to disk.", min, max, sum, sum/(wfsinfo->res.x * wfsinfo->res.y));
		
		// set new display settings to show the darkfield
		disp.dispsrc = DISPSRC_DARK;
		disp.dispover = 0;
		displayDraw(wfsinfo, &disp, &shtrack);
		snprintf(title, 64, "%s - Darkfield", disp.caption);
		SDL_WM_SetCaption(title, 0);
		
		// reset the display settings
		disp.dispsrc = oldsrc;
		disp.dispover = oldover;
	}
	else if (ptc->calmode == CAL_FLAT) {
		logInfo(0, "Starting flatfield calibration now");

		// set flat constant so we get a gain of 1
		gsl_matrix_float_set_all(wfsinfo->darkim, 32.0);
		
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
		float tmpavg;
		for (sn=0; sn < shtrack.nsubap; sn++) {
			for (i=0; i< shtrack.track.y; i++) {
				for (j=0; j< shtrack.track.x; j++) {
					tmpavg += (gsl_matrix_float_get(wfsinfo->flatim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j) - \
						gsl_matrix_float_get(wfsinfo->darkim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j));
				}
			}
		}
		tmpavg /= ((shtrack.cells.x * shtrack.cells.y) * (shtrack.track.x * shtrack.track.y));
		
		// make actual matrices from dark and flat
		uint16_t *darktmp = (uint16_t *) wfsinfo->dark;
		uint16_t *gaintmp = (uint16_t *) wfsinfo->gain;

		for (sn=0; sn < shtrack.nsubap; sn++) {
			for (i=0; i< shtrack.track.y; i++) {
				for (j=0; j< shtrack.track.x; j++) {
					darktmp[sn*(shtrack.track.x*shtrack.track.y) + i*shtrack.track.x + j] = \
						(uint16_t) (256.0 * gsl_matrix_float_get(wfsinfo->darkim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j));
					gaintmp[sn*(shtrack.track.x*shtrack.track.y) + i*shtrack.track.x + j] = (uint16_t) (256.0 * tmpavg / \
						(gsl_matrix_float_get(wfsinfo->flatim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j) - \
						 gsl_matrix_float_get(wfsinfo->darkim, shtrack.subc[sn].y + i, shtrack.subc[sn].x + j)));
				}
			}
		}

		logInfo(0, "Dark and gain fields initialized");
	}
	else if (ptc->calmode == CAL_SUBAPSEL) {
		logInfo(0, "Starting subaperture selection now");
		// no need to get get an image, it's alway the same for static simulation

		uint8_t *tmpimg = (uint8_t *) wfsinfo->image;
		uint8_t tmpmax = tmpimg[0];
		uint8_t tmpmin = tmpimg[0];
		uint64_t tmpsum, i;
		for (i=0; i<wfsinfo->res.x*wfsinfo->res.y; i++) {
			tmpsum += tmpimg[i];
			if (tmpimg[i] > tmpmax) tmpmax = tmpimg[i];
			else if (tmpimg[i] < tmpmin) tmpmin = tmpimg[i];
		}           
		logInfo(0, "Image info: sum: %ld, avg: %f, range: (%d,%d)", tmpsum, (float) tmpsum / (wfsinfo->res.x*wfsinfo->res.y), tmpmin, tmpmax);

		// run subapsel on this image
		modSelSubapts(wfsinfo->image, DATA_UINT8, ALIGN_RECT, &shtrack, wfsinfo);

		logInfo(0, "Subaperture selection complete, found %d subapertures.", shtrack.nsubap);
		// set new display settings to show the darkfield
		disp.dispsrc = DISPSRC_RAW;
		disp.dispover = DISPOVERLAY_SUBAPS | DISPOVERLAY_GRID;
		displayDraw(wfsinfo, &disp, &shtrack);
		snprintf(title, 64, "%s - Subaps", disp.caption);
		SDL_WM_SetCaption(title, 0);
		// reset the display settings
		disp.dispsrc = oldsrc;
		disp.dispover = oldover;
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
   col [i] [i] [i]:     change the overlay color (OpenGL only).\
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
			else if (strncmp(list[1], "cal",3) == 0) {
				tellClient(client->buf_ev, "\
200 OK HELP CALIBRATE\n\
calibrate <mode>:       calibrate the ao system.\n\
   dark:                take a darkfield by averaging %d frames.\n\
   flat:                take a flatfield by averaging %d frames.\n\
   gain:                calc dark/gain to do actual corrections with.\n\
   selsubap:            select some subapertures.\
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
resetdm [i]:            reset the DM to a certain voltage for all acts. def=0\n\
resetdaq [i]:           reset the DAQ analog outputs to a certain voltage. def=0\n\
set [prop]:             set or query certain properties.\n\
calibrate <mode>:       calibrate the ao system (dark, flat, subapt, etc).\
");
		}
	}
#ifdef FOAM_SIMDYN_DISPLAY
 	else if (strncmp(list[0], "disp",3) == 0) {
		if (count > 1) {
			if (strncmp(list[1], "raw",3) == 0) {
				tellClient(client->buf_ev, "200 OK DISPLAY RAW");
				disp.dispsrc = DISPSRC_RAW;
			}
			else if (strncmp(list[1], "cfull",3) == 0) {
				disp.dispsrc = DISPSRC_FULLCALIB;
				tellClient(client->buf_ev, "200 OK DISPLAY CALIB");
			}
			else if (strncmp(list[1], "cfast",3) == 0) {
				disp.dispsrc = DISPSRC_FASTCALIB;
				tellClient(client->buf_ev, "200 OK DISPLAY CALIB");
			}
			else if (strncmp(list[1], "grid",3) == 0) {
				logDebug(0, "overlay was: %d, is: %d, mask: %d", disp.dispover, disp.dispover ^ DISPOVERLAY_GRID, DISPOVERLAY_GRID);
				disp.dispover ^= DISPOVERLAY_GRID;
				tellClient(client->buf_ev, "200 OK TOGGLING GRID OVERLAY");
			}
			else if (strncmp(list[1], "subaps",3) == 0) {
				disp.dispover ^= DISPOVERLAY_SUBAPS;
				tellClient(client->buf_ev, "200 OK TOGGLING SUBAPERTURE OVERLAY");
			}
			else if (strncmp(list[1], "vectors",3) == 0) {
				disp.dispover ^= DISPOVERLAY_VECTORS;
				tellClient(client->buf_ev, "200 OK TOGGLING DISPLACEMENT VECTOR OVERLAY");
			}
			else if (strncmp(list[1], "col",3) == 0) {
				if (count > 4) {
					disp.col.r = strtol(list[2], NULL, 10);
					disp.col.g = strtol(list[3], NULL, 10);
					disp.col.b = strtol(list[4], NULL, 10);
					tellClient(client->buf_ev, "200 OK COLOR IS NOW (%d,%d,%d)", disp.col.r, disp.col.g, disp.col.b);
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
					tellClient(client->buf_ev, "200 OK DISPLAY DARK");
				}
			}
			else if (strncmp(list[1], "flat",3) == 0) {
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
			}
		}
		else {
			tellClient(client->buf_ev, "402 DISPLAY REQUIRES ARGS");
		}
	}
#endif
 	else if (strncmp(list[0], "set",3) == 0) {
		if (count > 2) {
			tmpint = strtol(list[2], NULL, 10);
			tmpfloat = strtof(list[2], NULL);
			if (strcmp(list[1], "lf") == 0) {
				ptc->logfrac = tmpint;
				tellClient(client->buf_ev, "200 OK SET LOGFRAC TO %d", tmpint);
			}
			else if (strcmp(list[1], "ff") == 0) {
				ptc->wfs[0].fieldframes = tmpint;
				tellClient(client->buf_ev, "200 OK SET FIELDFRAMES TO %d", tmpint);
			}
			else if (strcmp(list[1], "samini") == 0) {
				shtrack.samini = tmpfloat;
				tellClient(client->buf_ev, "200 OK SET SAMINI TO %.2f", tmpfloat);
			}
			else if (strcmp(list[1], "samxr") == 0) {
				shtrack.samxr = tmpint;
				tellClient(client->buf_ev, "200 OK SET SAMXR TO %d", tmpint);
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
samini:                 %.2f\n\
", ptc->logfrac, ptc->wfs[0].fieldframes, shtrack.cells.x, shtrack.cells.y,\
shtrack.shsize.x, shtrack.shsize.y, shtrack.track.x, shtrack.track.y, ptc->wfs[0].res.x, ptc->wfs[0].res.y, shtrack.samxr, shtrack.samini);
		}
	}
 	else if (strncmp(list[0], "vid",3) == 0) {
		if (count > 1) {
			if (strncmp(list[1], "auto",3) == 0) {
				disp.autocontrast = 1;
				tellClient(client->buf_ev, "200 OK USING AUTO SCALING");
			}
			else if (strcmp(list[1], "c") == 0) {
				if (count > 2) {
					tmpfloat = strtof(list[2], NULL);
					disp.autocontrast = 0;
					disp.contrast = tmpfloat;
					tellClient(client->buf_ev, "200 OK CONTRAST %f", tmpfloat);
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
				tellClient(client->buf_ev, "401 UNKNOWN VID");
			}
		}
		else {
			tellClient(client->buf_ev, "402 VID REQUIRES ARGS");
		}
	}
 	else if (strncmp(list[0], "cal",3) == 0) {
		if (count > 1) {
			if (strncmp(list[1], "dark",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_DARK;
                tellClient(client->buf_ev, "200 OK DARKFIELDING NOW");
				pthread_cond_signal(&mode_cond);
				// add message to the users
			}
			else if (strncmp(list[1], "sel",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_SUBAPSEL;
                tellClient(client->buf_ev, "200 OK SELECTING SUBAPTS");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "flat",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_FLAT;
				tellClient(client->buf_ev, "200 OK FLATFIELDING NOW");
				pthread_cond_signal(&mode_cond);
			}
			else if (strncmp(list[1], "gain",3) == 0) {
				ptc->mode = AO_MODE_CAL;
				ptc->calmode = CAL_DARKGAIN;
				tellClient(client->buf_ev, "200 OK CALCULATING DARK/GAIN NOW");
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

int drvSetActuator(control_t *ptc, int wfc) {
	// not a lot of actuating to be done for simulation
	if (ptc->wfc[wfc].type == 0) {			// Okotech DM
		// use okodm routines here
	}
	else if (ptc->wfc[wfc].type == 1) {	// Tip-tilt mirror
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

int MMAvgFramesByte(control_t *ptc, gsl_matrix_float *output, wfs_t *wfs, int rounds) {
	int k, i, j;
	float min, max, sum, tmpvar;
	uint8_t *imgsrc;
    logDebug(0, "Averaging %d frames now (dark, flat, whatever)", rounds);
	
	gsl_matrix_float_set_zero(output);
	for (k=0; k<rounds; k++) {
		if ((k % (rounds/10)) == 0 && k > 0)
			logDebug(0 , "Frame %d", k);
		
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
				
				tmp = (( ((uint16_t) tsrc[off+i*wfs->res.x + j]) << 8) - tdark[off+i*shtrack->track.x + j]); 
				
				// Here we check if src - dark < 0, and we set 
				// the pixel to zero if true
				if (tmp > (tsrc[off+i*wfs->res.x + j] << 8))
					tmp = 0;
					//tcorr[off+i*shtrack->track.x + j] = 0;
				// Here we check if we overflow the pixel range
				// after applying a gain, if so we set the pixel
				// to 255
				else if ((tmp = ((tmp * tgain[off+i*shtrack->track.x + j]) >> 16)) > 255)
					tmp = 255;
					//tcorr[off+i*shtrack->track.x + j] = 255;
				// if none of this happens, we just set the pixel
				// to the value that it should have
				else
					tmp = 1;
					//tcorr[off+i*shtrack->track.x + j] = tmp;
				
				// TvW 2008-07-02, directly copy raw to corr for the moment, this is statsim anyway
				// leaving the above intact will give a hint on the performance of this method though,
				// although the results of the above statements are thrown away
				tcorr[off+i*shtrack->track.x + j] = tsrc[i*wfs->res.x + j];//- tdark[off+i*shtrack->track.x + j];
			}
		}
	}
	float srcst[3];
	float corrst[3];
	imgGetStats(wfs->corr, DATA_UINT16, NULL, shtrack->nsubap * shtrack->track.x * shtrack->track.y, corrst);
	imgGetStats(wfs->image, DATA_UINT8, &(wfs->res), -1, srcst);
	//logDebug(LOG_SOMETIMES, "SUBCORR: corr: min: %f, max: %f, avg: %f", corrst[0], corrst[1], corrst[2]);
	//logDebug(LOG_SOMETIMES, "SUBCORR: src: min: %f, max: %f, avg: %f", srcst[0], srcst[1], srcst[2]);
	//sleep(1);
	
	return EXIT_SUCCESS;
}

// Dark flat calibration, currently only does raw-dark
int MMDarkFlatFullByte(wfs_t *wfs, mod_sh_track_t *shtrack) {
	logDebug(LOG_SOMETIMES, "Slow full-frame darkflat correcting now");
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
	float pix1, pix2;
	for (i=0; (int) i < wfs->res.y; i++) {
		for (j=0; (int) j < wfs->res.x; j++) {
			// pix 1 is flat - dark
			pix1 = (gsl_matrix_float_get(wfs->flatim, i, j) - \
					gsl_matrix_float_get(wfs->darkim, i, j));
			// pix 2 is max(raw - dark, 0)
			pix2 = fmax(imagesrc[i*wfs->res.x +j] - \
						gsl_matrix_float_get(wfs->darkim, i, j), 0);
			// if flat - dark is 0, we set the output to zero to prevent 
			// dividing by zero, otherwise we take max(pix2 / pix1, 255)
			// we multiply by 128 because (raw-dark)/(flat-dark) is
			// 1 on average (I guess), multiply by static 128 to get an image
			// at all. Actually this should be average(flat-dark), but that's
			// too expensive here, this should work fine :P
			if (pix1 <= 0)
				gsl_matrix_float_set(wfs->corrim, i, j, imagesrc[i*wfs->res.x +j]);
				//gsl_matrix_float_set(wfs->corrim, i, j, 0.0);
			else 
				gsl_matrix_float_set(wfs->corrim, i, j, imagesrc[i*wfs->res.x +j]);
				//gsl_matrix_float_set(wfs->corrim, i, j, \
									 fmin(128 * pix2 / pix1, 255));
			gsl_matrix_float_set(wfs->corrim, i, j, imagesrc[i*wfs->res.x +j]);
			
		}
	}
	float corrstats[3];
	float srcstats[3];
	imgGetStats(imagesrc, DATA_UINT8, &(wfs->res), -1, srcstats);
	imgGetStats(wfs->corrim, DATA_GSL_M_F, &(wfs->res), -1, corrstats);
	
	logDebug(LOG_SOMETIMES, "FULLCORR: src: min %f, max %f, avg %f", srcstats[0], srcstats[1], srcstats[2]);
	logDebug(LOG_SOMETIMES, "FULLCORR: corr: min %f, max %f, avg %f", corrstats[0], corrstats[1], corrstats[2]);

	return EXIT_SUCCESS;
}


int drvGetImg(control_t *ptc, int wfs) {
	if (simSensor(&simparams, &shtrack) != EXIT_SUCCESS) {
		logWarn("Error getting simulated wavefront.");
		return EXIT_FAILURE;
	}
	ptc->wfs[0].image = (void *) simparams.currimg;
	return EXIT_SUCCESS;
}
