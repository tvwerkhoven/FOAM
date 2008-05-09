/*! 
	@file foam_modules-itifg.c
	@author @authortim
	@date 2008-03-03 16:49

	@brief This file contains routines to read out a PCDIG framegrabber using the ITIFG driver.
*/

#include "foam_modules-itifg.h"

int drvInitBoard(mod_itifg_cam_t *cam) {
	int flags = O_RDWR | O_APPEND | O_SYNC; 
	int zero = 0;
	int one = 1;
	int result;	
		
	cam->fd = open(cam->device_name, flags);
	if (cam->fd == -1) {
		logWarn("Error opening device %s: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_DEBUG
	logDebug(0, "Camera device '%s' opened with flags %d, fd = %d", cam->device_name, flags, cam->fd);
#endif

// ???:tim:20080408 might not be necessary?
//	if (ioctl(cam->fd, GIOC_SET_LUT_LIN) < 0) {
//		close(cam->fd);
//		logWarn("%s: error linearising LUTs: %s", cam->device_name, strerror(errno));
//		return EXIT_FAILURE;
//	}

	if (ioctl(cam->fd, GIOC_SET_DEFCNF, NULL) < 0) {
		close(cam->fd);
		logWarn("%s: error setting camera configuration: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}	
	
	if (ioctl(cam->fd, GIOC_SET_CAMERA, &zero) < 0) {
		close(cam->fd);
		logWarn("%s: error setting camera: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (ioctl(cam->fd, GIOC_GET_CAMCNF, &(cam->itcam)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting camera configuration: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

	
	if ((result = iti_read_config(cam->config_file, &(cam->itcam), 0, cam->module, 0, cam->camera_name, cam->exo_name)) < 0) {
		close(cam->fd);
		logWarn("%s: error reading camera configuration from file %s: %s", cam->device_name, cam->config_file, strerror(errno));
		return EXIT_FAILURE;		
	}
#ifdef FOAM_DEBUG
	logDebug(0, "Read config '%s'. Camera: '%s', exo: '%s'", cam->config_file, cam->camera_name, cam->exo_name);
#endif
	
	if (ioctl(cam->fd, GIOC_SET_CAMCNF, &(cam->itcam)) < 0) {
		close(cam->fd);
		logWarn("%s: error setting camera configuration: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
//	if (ioctl(cam->fd, GIOC_SET_TIMEOUT, &(struct timeval){0, 0}) < 0) {
//		close(cam->fd);
//		logWarn("%s: error setting timeout: %s", cam->device_name, strerror(errno));
//		return EXIT_FAILURE;
//	}
//#ifdef FOAM_DEBUG
//	logDebug(0, "Timout set to {0,0}");
//#endif
	
	if(ioctl(cam->fd, GIOC_SET_HDEC, &one) < 0) {
		close(cam->fd);
		logWarn("%s: error setting horizontal decimation: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_SET_VDEC, &one) < 0) {
		close(cam->fd);
		logWarn("%s: error setting vertical decimation: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_DEBUG
	logDebug(0, "decimation set to {1,1} (i.e. we want full frames)");
#endif
	
	if(ioctl(cam->fd, GIOC_GET_WIDTH, &(cam->width)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting width: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_HEIGHT, &(cam->height)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting height: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_DEPTH, &(cam->depth)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting depth: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_DEBUG
	logDebug(0, "width x height x depth: %dx%dx%d", cam->width, cam->height, cam->depth);
#endif
	
	if(ioctl(cam->fd, GIOC_GET_RAWSIZE, &(cam->rawsize)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting raw size: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
	
	if(ioctl(cam->fd, GIOC_GET_PAGEDSIZE, &(cam->pagedsize)) < 0) {
		close(cam->fd);
		logWarn("%s: error getting paged size: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_DEBUG
	logDebug(0, "raw size: %d, paged size: %d", (int) cam->rawsize, (int) cam->pagedsize);
#endif
	
	if (fcntl(cam->fd, F_SETFL, fcntl(cam->fd, F_GETFL, NULL) & ~O_NONBLOCK) < 0) {
		close(cam->fd);
		logWarn("%s: error setting blocking: %s", cam->device_name, strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_DEBUG
	logDebug(0, "Camera configuration done.");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam) {
	
	// start mmap
	buf->map = mmap(NULL, cam->pagedsize * buf->frames, PROT_READ | PROT_WRITE, MAP_SHARED, cam->fd, 0);
	
	if (buf->map == (void *)-1) {
		logWarn("Could not mmap(): %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	buf->data = buf->map;
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);

#ifdef FOAM_DEBUG
	logDebug(0, "mmap() successful.");
#endif
	
	return EXIT_SUCCESS;
}

int drvInitGrab(mod_itifg_cam_t *cam) {
#ifdef FOAM_DEBUG
	logDebug(0, "Starting grab, lseeking to %ld.", +LONG_MAX);
#endif
	
	// start the framegrabber by seeking to +LONG_MAX
	if ( lseek(cam->fd, +LONG_MAX, SEEK_END) == -1) {
		logWarn("Error starting grab: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int drvStopGrab(mod_itifg_cam_t *cam) {
#ifdef FOAM_DEBUG
	logDebug(0, "Stopping grab, lseeking to %ld.", -LONG_MAX);
#endif
	// stop the framegrabber by seeking to -LONG_MAX
	if ( lseek(cam->fd, -LONG_MAX, SEEK_END) == -1) {
		logWarn("Error stopping grab: %s", strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int drvGetImg(mod_itifg_cam_t *cam, mod_itifg_buf_t *buf, struct timeval *timeout) {
	off_t seeke, seekc;
	int result;
#ifdef FOAM_DEBUG
//	logDebug(0, "Grabbing image...");
#endif
	fd_set in_fdset, ex_fdset;
	FD_ZERO (&in_fdset);
	FD_ZERO (&ex_fdset);
	FD_SET (cam->fd, &in_fdset);
	FD_SET (cam->fd, &ex_fdset);

	result = select(FOAM_MODITIFG_MAXFD, &in_fdset, NULL, &ex_fdset, timeout);
	
	if (result == -1)  {
		// error in select()
		logWarn("Select() returned no active FD's, error:%s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	if (result == 0) {
		// timeout occured, return immediately
		logInfo(0, "Timeout in drvGetImg(). Might be an error.");	
		return EXIT_SUCCESS;
	}

#ifdef FOAM_DEBUG
	//logDebug(0, "lseek 0 SEEK_CUR...");	
#endif
	seekc = lseek(cam->fd, 0, SEEK_CUR);
	if (seekc == -1) {
		logWarn("SEEK_CUR failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}

#ifdef FOAM_DEBUG
	//logDebug(0, "lseek 0 SEEK_END...");	
#endif
	seeke = lseek(cam->fd, 0, SEEK_END);
	if (seeke == -1) {
		logWarn("SEEK_END failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	
#ifdef FOAM_DEBUG
	//logDebug(0, "Select returned: %d, SEEK_CUR: %d, SEEK_END: %d", result, (int) seekc, (int) seeke);	
#endif

	// The next new image is located at the beginning of the buffer (buf->map)
	// plus an offset returned by SEEK_CUR, which must be wrapped around
	// the buffer size. SEEK_CUR returns an absolute offset wrt the beginning
	// of the mmap()ed memory, i.e. 45*paged_size. If the buffer is only
	// 4 frames big, the newest image is located at 
	// (((45*paged_size)/paged_size) % 4) * paged_size, which is 1* paged_size
	// info is located after the frame itself, so buf->data + rawsize
	buf->data = (void *)((char *)buf->map + ( (((int) seekc / cam->pagedsize) % buf->frames) * cam->pagedsize ) );
	buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);


#ifdef FOAM_DEBUG
	//logDebug(0, "lseek %d SEEK_CUR...", cam->pagedsize);	
#endif
	seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
	//seek = lseek(cam->fd, seek, SEEK_CUR);
	if (seekc == -1) {
		logWarn("SEEK_CUR failed: %s", strerror(errno));
		return EXIT_FAILURE;
	}
#ifdef FOAM_DEBUG
	//logDebug(0, "Select returned: %d, SEEK_CUR: %d", result, (int) seekc);	
#endif
	
	return EXIT_SUCCESS;
}

int drvStopBufs(mod_itifg_buf_t *buf, mod_itifg_cam_t *cam) {
	if (munmap(buf->map, cam->pagedsize * buf->frames) == -1) {
		logWarn("Error unmapping camera memory: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

int drvStopBoard(mod_itifg_cam_t *cam) {
	if (close(cam->fd) == -1) {
		logWarn("Unable to close framegrabber board fd: %s", strerror(errno));
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

#ifdef FOAM_MODITIFG_ALONE
int main(int argc, char *argv[]) {

	if (argc < 2)  {
		printf("Need config file! call <prog> <conffile.cam>\n");
		exit(-1);
	}
	else
		printf("Using conffile '%s'\n", argv[1]);

	// init vars
	int i, j, f;
	mod_itifg_cam_t camera;
	mod_itifg_buf_t buffer;
	char *file;
	
	// let's make ourselves important :)
	struct sched_param schedp;
	sched_getparam(0, &schedp);
	schedp.sched_priority = 50;
	if (sched_setscheduler(0,SCHED_FIFO, &schedp)) {
		printf("Unable to make ourselves important (i.e. raise prio)\n");
	}
	
	camera.module = 48; // some number taken from test_itifg
	camera.device_name = "/dev/ic0dma";
	camera.config_file = argv[1];

	buffer.frames = 4; // ringbuffer will be 4 frames big
	
	printf("This is the debug version for ITIFG8\n");
	printf("Trying to do something with '%s' using '%s'\n", camera.device_name, camera.config_file);
	
	// init cam
	if (drvInitBoard(&camera) != EXIT_SUCCESS)
		return EXIT_FAILURE;
	
	// init bufs
	if (drvInitBufs(&buffer, &camera) != EXIT_SUCCESS)
		return EXIT_FAILURE;

	// init display
	mod_display_t disp;
	disp.caption = "McMath - WFS";
	disp.res.x = (int) camera.width;
	disp.res.y = (int) camera.height;
	disp.flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
	disp.autocontrast = 0;
    disp.brightness = 0;
    disp.contrast = 20;

	modInitDraw(&disp);

	printf("Reseting framegrabber now...\n");
	lseek(camera.fd, -LONG_MAX, SEEK_END);

	coord_t res;
	res.x = (int) camera.width;
	res.y = (int) camera.height;

	off_t seekc, seeke;
	off_t seekco, overoff;
	//struct timeval timeout;
	struct timeval *timeout = NULL;
	int result;
	mod_itifg_buf_t *buf = &buffer;
	mod_itifg_cam_t *cam = &camera;
	fd_set in_fdset, ex_fdset;
	int pix, pixs;
	
	lseek(camera.fd, +LONG_MAX, SEEK_END);
    logDebug(0, "Giving 50 manual lseek images");
    
	// test images
	for (i=0; i<50; i++) {
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (camera.fd, &in_fdset);
		FD_SET (camera.fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
		printf("select: %d | ", result);
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));
		
		printf("lseek fd 0 seek_end: %d | ", (int) seeke);

		seekc = lseek(cam->fd, 0, SEEK_CUR);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));
		
		printf("lseek fd 0 seek_cur: %d | frame from %d to %d or %d to %d\n", (int) seekc, (int) seekc, (int) seeke, (int) seekc % cam->pagedsize, (int) seeke % cam->pagedsize);
		
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		modBeginDraw(disp.screen);
		modDisplayImgByte((uint8_t *) buf->data, &disp) ;
		modFinishDraw(disp.screen);
		printf("images: \n");
		for (f=0; f<buffer.frames; f++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		printf("lseek fd %d seek_cur: %d | END\n", cam->pagedsize, (int) seekc);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

		// reset frame capture
		/*
		if (seekc >= buffer.frames * camera.pagedsize)  {
			seeke = lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);
			printf("RESET: lseek fd %d SEEK_END: %d\n", buffer.frames * camera.pagedsize,(int) seeke);
		}
		*/
//		if (seekc >= buffer.frames * camera.pagedsize) 
//			lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);

		// reset frame capture
		/*
		if (seekc >= buffer.frames * camera.pagedsize) 
			lseek(camera.fd, buffer.frames * camera.pagedsize, SEEK_END);
		*/
		
	}
	lseek(camera.fd, -LONG_MAX, SEEK_END);
	
    logDebug(0, "Giving 10000 images using the module API, displaying every 100th");
    struct timeval last, cur;
    gettimeofday(&last, NULL);
    float fps;
    
    drvInitGrab(cam);
    for (i=0; i<500; i++) {
        
        drvGetImg(cam, buf, NULL);
        
        if ((i % 100) == 0) {
    		gettimeofday(&cur, NULL);
            fps = (cur.tv_sec * 1000000 + cur.tv_usec)- (last.tv_sec*1000000 + last.tv_usec);
	    fps = 100*1000000/fps;
            logDebug(0, "Drawing image, fps: %f ", fps);
            last = cur;
            modBeginDraw(disp.screen);
            modDisplayImgByte((uint8_t *) buf->data, &disp) ;
            modFinishDraw(disp.screen);
        }
        
    }
    drvStopGrab(cam);
    
	
	/*
	lseek(camera.fd, +LONG_MAX, SEEK_END);

	printf("Starting long run\n");
	int overcnt = 0;
	// test images
	for (i=0; i<30840 + 40*16384; i++) {
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (camera.fd, &in_fdset);
		FD_SET (camera.fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
		if (overcnt < 5)
			printf("frame: %d | select: %d | ", i, result);

		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));
		
		if (overcnt < 5)
			printf("lseek fd 0 seek_end: %d | ", (int) seeke);

		seekc = lseek(cam->fd, 0, SEEK_CUR);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));
		if (seekc < seekco) {
			printf("Overflow at frame %d! from %d to %d, diff %d\n",i, (int)seekco, (int) seekc, (int) (seekco-seekc));
			overcnt = 0;
		}

		seekco = seekc;
		if (overcnt < 5) {
			printf("lseek fd 0 seek_cur: %d | ", (int) seekc);
			printf("frame from %d to %d or pos %d | ", (int) seekc,(int) seeke,(int) (seekc / cam->pagedsize) % buffer.frames);
		}
		
		
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		if (overcnt < 5) {
			printf("images: \n");
			for (f=0; f<buffer.frames; f++) {
				pixs = 0;
				for (j=0; j<25; j++) { 
					pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
					pixs += pow(2,j) * pix;
					printf("%d,", pix);
				}
				printf("H: %d\n", (int) sqrt(pixs));
			}
			overcnt++;
		}
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		if (overcnt < 5) 
			printf("lseek fd %d seek_cur: %d | END\n\n", cam->pagedsize, (int) seekc);

		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

	}
	lseek(camera.fd, -LONG_MAX, SEEK_END);

	exit(0);

	printf("Testing various lseek combinations...\n");
	printf("***************************************\n");
		
	printf("\nseek_end 0 / seek_cur pagedsize / buf->data = buf->map...\n");
	printf("****************************************\n");
	for (i=0; i<5; i++) {

		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (cam->fd, &in_fdset);
		FD_SET (cam->fd, &ex_fdset);

		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);

		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	

		printf("select: %d | ", result);
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));

		printf("0 seek_end: %d | ", (int) seeke);
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
			
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		printf("%d seek_cur: %d | ", cam->pagedsize, (int) seekc);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

		printf("image: \n");
		for (f=0; f<buffer.frames; f++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d", (int) sqrt(pixs));
			
			printf("\n");
		}
		//sleep(1);
	}
		
	printf("\n");
	drvStopGrab(&camera);
	drvInitGrab(&camera);
	
	printf("\nseek_end 0 / buf->data = buf->map / seek_cur pagedsize...\n");
	printf("doing SEEK_END, read image, wait 1 sec (buffer fills?), read image, \
		   do SEEK_CUR, read image, wait 1 sec (buffer fills?), read image, loop\n");
	printf("****************************************\n");
	for (i=0; i<3; i++) {
		
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (cam->fd, &in_fdset);
		FD_SET (cam->fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
//		printf("select: %d | ", result);
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));

		printf("0 seek_end: %d |\n", (int) seeke);
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
		sleep(1);
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}

		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		printf("%d seek_cur: %d |\n", cam->pagedsize, (int) seekc);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
		sleep(1);
		
		for (i=0; i<2; i++) {
			pixs = 0;
			for (j=0; j<25; j++) { 
				pix = *( ((unsigned char *) (buffer.data)) + j); 
				pixs += pow(2,j) * pix;
				printf("%d,", pix);
			}
			printf("H: %d\n", (int) sqrt(pixs));
		}
		
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
	}
	
	printf("\n");
	//	drvStopGrab(&camera);
	//	drvInitGrab(&camera);
	
//	printf("seek_end 0 / seek_cur <seek_end out> / buf->data = buf->map...\n");
//	printf("****************************************\n");
//	for (i=0; i<5; i++) {
//		FD_ZERO (&in_fdset);
//		FD_ZERO (&ex_fdset);
//		FD_SET (cam->fd, &in_fdset);
//		FD_SET (cam->fd, &ex_fdset);
//		
//		//result = poll(&pfd, 1, timeout);
//		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
//		
//		if (result == -1)
//			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
//		else if (result == 0)
//			printf("Timeout in drvGetImg().\n");	
//		
//		printf("select: %d | ", result);
//		seeke = lseek(cam->fd, 0, SEEK_END);
//		if (seeke == -1)
//			printf("SEEK_END failed: %s\n", strerror(errno));
//		
//		printf("0 seek_end: %d | ", (int)  seeke);
//		buf->data = (void *)((char *)buf->map);
//		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
//		
//		seekc = lseek(cam->fd, seeke, SEEK_CUR);
//		printf("%d seek_cur: %d | ", (int) seeke, (int) seekc);
//		if (seekc == -1)
//			printf("SEEK_CUR failed: %s\n", strerror(errno));
//		
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
//	}
//	
//	
//	printf("\n");
//	drvStopGrab(&camera);
//	drvInitGrab(&camera);
//	printf("seek_end <seek_end out> / seek_cur pagedsize / buf->data = buf->map...\n");
//	printf("****************************************\n");
//	// old seek_out value, init to 0
//	off_t seekeo = (off_t) 0;
//	for (i=0; i<5; i++) {
//		FD_ZERO (&in_fdset);
//		FD_ZERO (&ex_fdset);
//		FD_SET (cam->fd, &in_fdset);
//		FD_SET (cam->fd, &ex_fdset);
//		
//		//result = poll(&pfd, 1, timeout);
//		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
//		
//		if (result == -1)
//			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
//		else if (result == 0)
//			printf("Timeout in drvGetImg().\n");	
//		
//		printf("select: %d | ", result);
//		seeke = lseek(cam->fd, seekeo, SEEK_END);
//		if (seeke == -1)
//			printf("SEEK_END failed: %s\n", strerror(errno));
//		
//		printf("%d seek_end: %d | ", (int) seekeo, (int) seeke);
//		
//		// store seek_out output for use in next loop
//		seekeo = seeke;
//		buf->data = (void *)((char *)buf->map);
//		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
//		
//		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
//		printf("%d seek_cur: %d | ", cam->pagedsize, (int) seekc);
//		if (seekc == -1)
//			printf("SEEK_CUR failed: %s\n", strerror(errno));
//		
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
//	}
//
//	drvStopGrab(&camera);
//	drvInitGrab(&camera);
//	printf("\nseek_end 0 / seek_cur <disabled> / buf->data = buf->map...\n");
//	printf("****************************************\n");
//	for (i=0; i<5; i++) {
//
//		FD_ZERO (&in_fdset);
//		FD_ZERO (&ex_fdset);
//		FD_SET (cam->fd, &in_fdset);
//		FD_SET (cam->fd, &ex_fdset);
//
//		//result = poll(&pfd, 1, timeout);
//		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
//
//		if (result == -1)
//			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
//		else if (result == 0)
//			printf("Timeout in drvGetImg().\n");	
//
//		printf("select: %d | ", result);
//		seeke = lseek(cam->fd, 0, SEEK_END);
//		if (seeke == -1)
//			printf("SEEK_END failed: %s\n", strerror(errno));
//
//		printf("0 seek_end: %d | ", (int) seeke);
//		buf->data = (void *)((char *)buf->map);
//		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
//			
//		//seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
//		printf("%d seek_cur: <disabled> | ", cam->pagedsize);
//		//if (seekc == -1)
//			//printf("SEEK_CUR failed: %s\n", strerror(errno));
//
//		printf("image: \n");
//		for (f=0; f<buffer.frames; f++) {
//			pixs = 0;
//			for (j=0; j<25; j++) { 
//				pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
//				pixs += pow(2,j) * pix;
//				printf("%d,", pix);
//			}
//			printf("H: %d", (int) sqrt(pixs));
//			
//			printf("\n");
//		}
//	}
//		
//	printf("\n");
//	printf("cleaning up now\n");
	
	//drvStopGrab(&camera);
	//drvInitGrab(&camera);
	printf("\nseek_end 0 / seek_cur pagedsize / buf->data = buf->map... long run!\n");
	printf("****************************************\n");
	// no buffering 
	setvbuf(stdout, NULL, _IONBF, 0);
	for (i=0; i<31000; i++) {
		
		FD_ZERO (&in_fdset);
		FD_ZERO (&ex_fdset);
		FD_SET (cam->fd, &in_fdset);
		FD_SET (cam->fd, &ex_fdset);
		
		//result = poll(&pfd, 1, timeout);
		result = select(1024, &in_fdset, NULL, &ex_fdset, timeout);
		
		if (result == -1)
			printf("Select() returned no active FD's, error:%s\n", strerror(errno));
		else if (result == 0)
			printf("Timeout in drvGetImg().\n");	
		
		seeke = lseek(cam->fd, 0, SEEK_END);
		if (seeke == -1)
			printf("SEEK_END failed: %s\n", strerror(errno));
		
		buf->data = (void *)((char *)buf->map);
		buf->info = (iti_info_t *)((char *)buf->data + cam->rawsize);
		
		seekc = lseek(cam->fd, cam->pagedsize, SEEK_CUR);
		if (seekc == -1)
			printf("SEEK_CUR failed: %s\n", strerror(errno));

		if (i % 100 == 0)
			printf(".");
		if (i > 30000) 
			printf("i: %d, end: %d, cur: %d\n", i, (int) seeke, (int) seekc);
	}
	printf("after 31k images: seek_end: %d, seek_cur: %d \n", (int) seeke, (int) seekc);
	printf("buffer looks like (pagedsize boundaries):\n");
	
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.pagedsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}

	printf("buffer looks like (iti_info_t appendix (first 25 bytes)):\n");
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + camera.rawsize + j + f*camera.pagedsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}

	printf("buffer looks like (iti_info_t appendix (first 25 bytes)):\n");
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + camera.rawsize + j + f*camera.pagedsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}
	
	
	printf("buffer looks like (rawsize boundaries):\n");
	for (f=0; f<buffer.frames; f++) {
		for (j=0; j<25; j++) { 
			pix = *( ((unsigned char *) (buffer.data)) + j + f*camera.rawsize); 
			printf("%d,", pix);
		}
		printf("\n");
	}
	*/
	
	printf("\n");
	printf("cleaning up now\n");
	
	// cleanup
	drvStopGrab(&camera);
	drvStopBufs(&buffer, &camera);
	
	// end
	modFinishDraw(disp.screen);
	printf("exit\n");
	
	return 0;
}
#endif


