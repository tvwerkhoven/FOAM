/* 
	@file foam_library.c (was ao_library.c)
	@brief Library file for the whole AO framework
	@author @authortim
	@date November 13 2007
*/

#include "foam_library.h"

int printUTCDateTime(char **ret) {
	struct tm *utc;
	time_t t;
	t = time(NULL);
	utc = gmtime(&t);
 	*ret = asctime(utc);
	return EXIT_SUCCESS;
}

int sendMsg(const int sock, const char *buf) {
	printf("sending, len: %d", (int) strlen(buf));
	return write(sock, buf, strlen(buf)); // TODO non blocking maken
}
