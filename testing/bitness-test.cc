#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main() {
	int *dataint = new int(32);
	uint32_t *data32 = new uint32_t(32);
	uint64_t *data64 = new uint64_t(32);
	
	printf("sizeof int: %zu\n", sizeof (int));
	printf("sizeof int*: %zu\n", sizeof (int*));

	printf("sizeof uint32_t: %zu\n", sizeof (uint32_t));
	printf("sizeof uint32_t*: %zu\n", sizeof (uint32_t*));
	
	printf("sizeof uint64_t: %zu\n", sizeof (uint64_t));
	printf("sizeof uint64_t*: %zu\n", sizeof (uint64_t*));
	
	printf("This is a %d bit system.\n", sizeof (int*)*8);
	return 0;
}
