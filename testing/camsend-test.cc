#include <unistd.h>
#include <string>
#include <sigc++/signal.h>
#include <fitsio.h>

#include "protocol.h"
#include "socket.h"
#include "pthread++.h"

using namespace std;
typedef Protocol::Server::Connection Connection;

void on_client_msg(string line);

int main(int argc, char *argv[]) {
	printf("%d.\n", argc);
	if (argc < 2)
		return fprintf(stderr, "%s <file.fits>\n", argv[0]);
	
	fprintf(stderr,"Load FITS file...\n");
	fitsfile *fptr;
	char fits_err[30];
	int stat = 0, naxis;
	long naxes[8], nel = 0, fpixel[] = {1,1};
	int anynul = 0;
	int bpp=0;
	
	fits_open_file(&fptr, argv[1], READONLY, &stat);
	fits_get_img_param(fptr, 8, &bpp, &naxis, naxes, &stat);
	
	if (stat)
		return fprintf(stderr, "FITS error, aborting.\n");
	
	if (bpp != BYTE_IMG || naxis != 2)
		return fprintf(stderr, "Only 2D 8bpp images supported.\n");
	
	int w = naxes[0], h = naxes[1];
	int size = w*h;
	void *data = (void *) malloc(size);
	fprintf(stderr, "Got %d x %d image, loading...\n", w, h);
	
	uint8_t nulval = 0;
	fits_read_img(fptr, TBYTE, 1, size, &nulval, \
				  (uint8_t *) (data), &anynul, &stat);
	
	fits_close_file(fptr, &stat);
	
	if (stat)
		return fprintf(stderr, "FITS errror, aborting.\n");
	
	fprintf(stderr,"Got data, send to camview...\n");
	
	Protocol::Client client("127.0.0.1","1234", "CAM");
	client.connect();
	client.slot_message = sigc::ptr_fun(on_client_msg);
	sleep(1);
	
	string msg = "hello world";
	fprintf(stderr, "client.write(\"%s\");\n", msg.c_str());
	client.write(msg);
	
	fprintf(stderr, "sending image...\n");
	client.write(format("IMG %zu %d %d %d %d %d", w*h, 0, 0, w, h, 1) + "");
	client.write(data, w*h);
	
	sleep(2);

	return 0;
}


void on_client_msg(string line) {
	fprintf(stderr, "cli:on_client_msg: %s\n", line.c_str());
}
