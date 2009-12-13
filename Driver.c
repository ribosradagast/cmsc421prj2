#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

/*
* parameters : address to buffer, bucket ID
*/
int main() {

	/* put process 1 and 2 in 1 bucket */
	printf("Starting process 1 in bucket 1\n");
	execlp("./process1.out","./process1.out", "1", (char *) 0);
	printf("Starting process 2 in bucket 1\n");
	execlp("./process2.out","./process2.out", "1", (char *) 0);

	/* put process 3 in a new bucket (-1) */
	printf("Starting process 3 in a new bucket\n");
	execlp("./process3.out","./process3.out", "-1", (char *) 0);

	/* put process 4 in a new bucket (4) */
	printf("Starting process 4 in bucket 4\n");
	execlp("./process4.out","./process4.out", "4", (char *) 0);

	printf("Main program has terminated\n");
	return 0;
}


