#ifndef __IMGIO_H__
#define __IMGIO_H__

#include <string>
#include "types.h"

// Datatypes
#define IMGIO_FITS				0x00000001
#define IMGIO_PGM					0x00000001

// Image types
#define IMGIO_BYTE			7
#define IMGIO_UBYTE			8
#define IMGIO_SHORT			15
#define IMGIO_USHORT		16

class Imgio {
	std::string path;
	
	int dtype, bitpix;
	coord_t res;
	void *data;
	
	std::string err;
public:
	Imgio::Imgio(std::string, int);
	~Imgio(void);
	int Imgio::loadImg();
	int Imgio::writeImg(int, std::string);
	void *Imgio::getData() { return data; }
	int Imgio::getWidth() { return res.x; }
	int Imgio::getHeight() { return res.y; }
	int Imgio::getDtype() { return dtype; }
	int Imgio::getBitpix() { return bitpix; }
private:
	int Imgio::loadFits(std::string);
	int Imgio::writeFits(std::string);
	int Imgio::loadPgm(std::string);
	int Imgio::writePgm(std::string);
};

static int readNumber(FILE *fd);

#endif
