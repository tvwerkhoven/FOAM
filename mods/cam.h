#ifndef __CAM_H_
#define __CAM_H_

#include <string>
#include "imgio.h"
#include "types.h"

// Camera types
#define CAM_STATIC				0x00000001  // Static image

// Camera datatype
typedef struct {
	int type;
	img_t *img;
} cam_t;


typedef struct {
	void *data;
	coord_t res;
	int stride;
	int bitpix;
	int dtype;
} img_t;

class Cam {
	cam_t *cam;
	int setup;
	
public:
	Cam::Cam(int);
	~Cam(void);
	int Cam::init();
	int Cam::reconf();
	int Cam::getFrame();
};

static int readNumber(FILE *fd);

#endif /* __CAM_H_ */
