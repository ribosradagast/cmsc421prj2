#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

/*
* parameters : address to buffer, bucket ID
*/
int main(int argc, char *argv[]) {
	
	if ( argc != 3 ) /* argc should be 2 for correct execution */
	{
		/* We print argv[0] assuming it is the program name */
		printf( "You should have specified a string to append to and an integer for bucket ID\n");
		return -1;
	}
	else 
	{

		
		
		int i=0;
		struct sched_param  params;
		params.sched_priority=42;
		sched_setscheduler(getpid(), atoi( argv[2]), &params);
		for(i=0; i<100; i++){
			strcat(argv[1], "Program 1 just ran for the ");
			argv[1]+=i;
			strcat(argv[1], "th time.\n");
			
			sleep(1);
		}
		return 0;
	}
	
}
