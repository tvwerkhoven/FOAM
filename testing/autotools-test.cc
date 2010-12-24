/*!
 autotools-test.cc -- test autotools variables
 */

#include <stdio.h>
#include "autoconfig.h"

int main() {
	printf("Hello World\n");
	printf("Package name: %s\n", PACKAGE_NAME);
	printf("Package version: %s\n", PACKAGE_VERSION);
	printf("Package bugreport: %s\n", PACKAGE_BUGREPORT);
#ifdef GIT_REVISION
	printf("GIT_REVISION: %s\n", GIT_REVISION);
#endif
}

