#ifndef __IMGIO_H__
#define __IMGIO_H__

#include <string>
#include "types.h"


class Imgio {
private:
	int loadFits(std::string);
	int writeFits(std::string);
	int loadPgm(std::string);
	int writePgm(std::string);
	
	int calcRange();
	
	std::string path;
	
public:
	// Image formats
	typedef enum {
		FITS=0,
		PGM,
		UNDEF
	} imgtype_t;
	
	void *data;
	coord_t res;
	int bpp;
	
	uint16_t range[2];
	uint64_t sum;
	
	dtype_t dtype;
	imgtype_t imgt;
	
	std::string strerr;
	
	Imgio(void) { init("", Imgio::UNDEF); }
	Imgio(std::string f, imgtype_t t) { init(f, t); }
	~Imgio(void);
	
	int init(std::string, imgtype_t);
	int loadImg();
	int writeImg(imgtype_t, std::string);
	
	void *getData() { return data; }
	int getPixel(int x, int y);
	
	int getWidth() { return res.x; }
	int getHeight() { return res.y; }
	dtype_t getDtype() { return dtype; }
	int getBitpix() { return bpp; }
	int getBPP() { return bpp; }
	imgtype_t getImgtype() { return imgt; }
};

int readNumber(FILE *fd);

#endif
