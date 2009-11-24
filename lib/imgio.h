#ifndef __IMGIO_H__
#define __IMGIO_H__

#include <string>
#include "types.h"

// Datatypes
#define IMGIO_FITS				0x00000001
#define IMGIO_PGM					0x00000002

// Image types
#define IMGIO_BYTE			7
#define IMGIO_UBYTE			8
#define IMGIO_SHORT			15
#define IMGIO_USHORT		16


typedef struct {
	void *data;
	coord_t res;
	int stride;
	int bitpix;
	int dtype;
} img_t;

class Imgio {
	std::string path;
	img_t *img;
public:
	std::string strerr;
	
	Imgio(std::string, int);
	~Imgio(void);
	int loadImg();
	int writeImg(int, std::string);
	void *getData() { return img->data; }
	int getWidth() { return img->res.x; }
	int getHeight() { return img->res.y; }
	int getDtype() { return img->dtype; }
	int getBitpix() { return img->bitpix; }
private:
	int loadFits(std::string);
	int writeFits(std::string);
	int loadPgm(std::string);
	int writePgm(std::string);
};

static int readNumber(FILE *fd);

#endif
