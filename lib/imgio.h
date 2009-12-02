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
	int stride;
	int bitpix;
	
	dtype_t dtype;
	imgtype_t imgt;
	
	std::string strerr;
	
	Imgio::Imgio(void);
	Imgio(std::string, imgtype_t);
	~Imgio(void);
	int loadImg();
	int writeImg(imgtype_t, std::string);
	
	void *getData() { return data; }
	int getWidth() { return res.x; }
	int getHeight() { return res.y; }
	dtype_t getDtype() { return dtype; }
	int getBitpix() { return bitpix; }
	imgtype_t getImgtype() { return imgt; }
};

int readNumber(FILE *fd);

#endif
