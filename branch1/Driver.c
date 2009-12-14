#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/sched.h>
#include <string.h>
#include <linux/kernel.h>
#include <sys/types.h>

/* Spawn a child process running a new program. PROGRAM is the name
of the program to run; the path will be searched for this program.
ARG_LIST is a NULL-terminated list of character strings to be
passed as the program’s argument list. Returns the process ID of
the spawned process. */
int spawn (char* program, char** arg_list)
{
	pid_t child_pid;
	/* Duplicate this process. */
	child_pid = fork ();
	if (child_pid != 0)
	/* This is the parent process. */
	return child_pid;
	else {
		/* Now execute PROGRAM, searching for it in the path. */
		execvp(program, arg_list);
		/* The execvp function returns only if an error occurs. */
		fprintf (stderr, "an error occurred in execvp\n");
		abort ();
	}
}

/*
* parameters : address to buffer, bucket ID
*/
int main() {

char* arg_list[]={"./process1.out", "1", (char *) 0};
	/* put process 1 and 2 in 1 bucket */
	/*printk(KERN_ALERT "Starting process 1 in bucket 1\n");*/
	printf("Starting process 1 in bucket 1\n");
	spawn("./process1.out", arg_list);
	
	
	
char* arg_list2[]= {"./process2.out", "1", (char *) 0};
	/*printk(KERN_ALERT "Starting process 2 in bucket 1\n");*/
	printf("Starting process 2 in bucket 1\n");
	spawn("./process2.out", arg_list2);

	/* put process 3 in a new bucket (-1) */
char* arg_list3[]= {"./process3.out", "-1", (char *) 0};
	/*printk(KERN_ALERT "Starting process 4 in a new bucket\n");*/
	printf("Starting process 3 in a new bucket\n");
	spawn("./process3.out", arg_list3);

	/* put process 4 in a new bucket (4) */
char* arg_list4[]= {"./process4.out", "4", (char *) 0};
	/*printk(KERN_ALERT "Starting process 4 in bucket 4\n");*/
	printf("Starting process 4 in bucket 4\n");
	spawn("./process4.out", arg_list4);

	printf("Main program has terminated\n");
	return 0;
}



