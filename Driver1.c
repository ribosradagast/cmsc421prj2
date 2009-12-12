#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

/*
* parameters : address to buffer, bucket ID
*/
int main(int argc, char *argv[]) {
	
	int i=0;
	struct sched_param  params;
	params.sched_priority=42;
	sched_setscheduler(getpid(), atoi( argv[2]), &params);
	for(i=0; i<100; i++){
		strcat(argv[1], "Program 1 just ran for the ");
		strcat(argv[1], atoi(i));
		strcat(argv[1], "th time.\n");
		
		sleep(1);
	}
return 0;
}