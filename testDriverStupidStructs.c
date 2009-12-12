#include <stdlib.h>

struct rt_rq{

/*
array for bucket-numofcontents
numInBucket[i] is the number of processes that have bucket id
'i' (in bucket 'i')
*/
int numInBucket[1000];
} ;



static void init_rt_rq(struct rt_rq *rt_rq);

static void init_rt_rq(struct rt_rq *rt_rq)
{
int i;

/*
initialize  array for bucket num_contents
TODO: see if this works
*/
for (i = 0; i < 1000; i++) {
printf("array at %d initialised to 0\n", i);
rt_rq->numInBucket[i]=0;
}

rt_rq *ourRQ;

int main()
{
init_rt_rq(ourRQ);

return 0;
}

