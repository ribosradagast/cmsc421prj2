#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <string.h>

/*
* parameters : address to buffer, bucket ID
*/
int main(int argc, char *argv[]) {
	
	if ( argc != 2 ) /* argc should be 2 for correct execution */
	{
		/* We print argv[0] assuming it is the program name */
		printf( "You should have specified a string to append to and an integer for bucket ID\n");
		return -1;
	}
	else 
	{
		int i=0;
		
		int j=0;
		int k=0;
		struct sched_param  params;
		params.sched_priority=atoi( argv[2]);
		sched_setscheduler(getpid(),   6 , &params);
		for(i=0; i<10; i++){
			printf("Program 3 just started execution for the %dth time\n", i);
			for(j=0; j<100000; j++){
				for(k=0; k<1000; k++){
					j*k;
				}
			}
			
			printf("Program 3 just finished execution for the %dth time\n", i);
			argv[1]+=i;
			strcat(argv[1], "th time.\n");
			
		}
		return 0;
	}
}