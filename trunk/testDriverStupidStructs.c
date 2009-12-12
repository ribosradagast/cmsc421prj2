#include <stdlib.h>
#include <stdio.h>



struct bucketArray{
	/*
array for bucket-numofcontents
numInBucket[i] is the number of processes that have bucket id
'i' (in bucket 'i')
*/
int numInBucket [5000];
} ;



void init_rt_rq(struct bucketArray *rt_rq);

void init_rt_rq(struct bucketArray *rt_rq)
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
	struct bucketArray *ourRQ=(struct bucketArray *)malloc(sizeof(struct bucketArray)*1000);

	init_rt_rq(ourRQ);

	return 0;
}

