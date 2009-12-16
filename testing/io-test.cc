#include <stdio.h>
#include "io.h"

int main() {
	Io *io;

	printf("Test printing at different error levels...\n");
	for (int i=0; i<IO_MAXLEVEL+1; i++) {
		io = new Io(i);
		printf("==== Error level = %d\n", io->getVerb());

		io->msg(IO_ERR, "Error");
		io->msg(IO_WARN, "Warn");
		io->msg(IO_INFO, "Info");
		io->msg(IO_XNFO, "Xnfo");
		io->msg(IO_DEB1, "Debug1");
		io->msg(IO_DEB2, "Debug2");
		delete io;
	}
	
	printf("Test level incrementing and decrementing...\n");
	io = new Io(1);
	
	if (io->getVerb() != 1)
		printf("ERROR: initial level wrong!\n");
	
	// Increment a few times
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	
	if (io->getVerb() != 7 && io->getVerb() != IO_MAXLEVEL)
		printf("ERROR: incrementing failed!\n");

	// Decrement more
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	io->decVerb();
	
	if (io->getVerb() != 1)
		printf("ERROR: decrementing failed!\n");
	
	delete io;
	
	printf("Done!\n");
	return 0;
}

