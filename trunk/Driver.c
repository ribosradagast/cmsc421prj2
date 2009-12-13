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
	execlp("process1.out","process1.out", 1);
	execlp("process2.out","process2.out", 1);

	/* put process 3 in a new bucket (-1) */
	execlp("process3.out","process3.out", -1);

	/* put process 4 in a new bucket (4) */
	execlp("process4.out","process4.out", 4);

	printf("Main program has terminated");
}


