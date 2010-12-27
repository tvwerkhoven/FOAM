/*!
 autotools-test.cc -- test autotools variables
 */

#ifdef HAVE_CONFIG_H
#include "autoconfig.h"
#endif

#include <stdio.h>

int main() {
	printf("Hello World\n");
	printf("Package name: %s\n", PACKAGE_NAME);
	printf("Package version: %s\n", PACKAGE_VERSION);
	printf("Package bugreport: %s\n", PACKAGE_BUGREPORT);
#ifdef GIT_REVISION
	printf("GIT_REVISION: %s\n", GIT_REVISION);
#endif
}

