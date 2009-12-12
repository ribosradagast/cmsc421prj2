#include <stdlib.h>
#include <stdio.h>



struct rt_rq{
	/*
array for bucket-numofcontents
numInBucket[i] is the number of processes that have bucket id
'i' (in bucket 'i')
*/
int *numInBucket ;
} ;



void init_rt_rq(struct rt_rq *rt_rq);

void init_rt_rq(struct rt_rq *rt_rq)
{
	int i;

	/*
initialize  array for bucket num_contents
TODO: see if this works
*/
	for (i = 0; i < 1000; i++) {
	/*	rt_rq->numInBucket[i]=0;*/
		printf("array at %d initialised to %d\n", i, rt_rq->numInBucket[i]);
	}
}

int main()
{
	struct rt_rq *ourRQ={0};
	ourRQ -> numInBucket = (int *)malloc(sizeof(int)*1000);

	init_rt_rq(ourRQ);

	return 0;
}

