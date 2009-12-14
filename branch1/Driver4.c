#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/kernel.h>
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
		params.sched_priority=atoi( argv[1]);
		printf("Attempting to set the scheduler for Program 4...\n");
		i=sched_setscheduler(getpid(),   6 , &params);
		printf("Scheduler has been set for for Program 4! Return value is: %d\n", i);
		for(i=0; i<10; i++){
			/*printk(KERN_ALERT "Program 4 just started execution for the %dth time\n", i);*/
			printf("Program 4 just started execution for the %dth time\n", i);
			for(j=0; j<100000; j++){
				for(k=0; k<5000; k++){
					j*k;
				}
			}
			
			/*printk(KERN_ALERT "Program 4 just finished execution for the %dth time\n", i);*/
			printf("Program 4 just finished execution for the %dth time\n", i);
						
		}
		return 0;
	}
}
