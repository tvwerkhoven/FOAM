#include <stdio.h>
#include "Io.h"

int main() {
	Io *io;

	for (int i=0; i<10; i++) {
		io = new Io(i);
		printf("==== Level = %d = %d\n", i, io->getVerb());

		io->msg(IO_ERR, "Error");
		io->msg(IO_WARN, "W");
		io->msg(IO_INFO, "I");
		io->msg(IO_XNFO, "X");
		io->msg(IO_DEB1, "1");
		io->msg(IO_DEB2, "2");
		delete io;
	}

	io = new Io(0);
	printf("==== Level = %d = %d\n", 0, io->getVerb());
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	io->incVerb();
	printf("==== Level = %d = %d\n", 7, io->getVerb());
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
	printf("==== Level = %d = %d\n", -2, io->getVerb());
	
	delete io;
	
	return 0;
}

