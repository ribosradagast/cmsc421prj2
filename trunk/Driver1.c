#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <strings.h>

/*
* parameters : address to buffer, bucket ID
*/
int main(int argc, char *argv[]) {
	
	int i=0;
	struct sched_param  params;
	params.sched_priority=42;
	sched_setscheduler(getpid(), atoi( argv[2]), &params);
	for(i=0; i<100; i++){
		strcat(argv[1], "Program 1 just ran for the "+i+"th time.\n");
		strcat(argv[1], (char)i);
		strcat(argv[1], "th time.\n");
		
		sleep(1);
	}

}
