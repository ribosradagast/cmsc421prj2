typedef struct {

/*
array for bucket-numofcontents
numInBucket[i] is the number of processes that have bucket id
'i' (in bucket 'i')
*/
int numInBucket[MAX_BRR_PRIO];
}rt_rq ;



static void init_rt_rq(struct rt_rq *rt_rq)
{
struct rt_prio_array *array;
int i;

#ifdef CONFIG_BRR_GROUP_SCHED
/*
initialize  array for bucket num_contents
TODO: see if this works
*/
for (i = 0; i < 1000; i++) {
printf("array at %d initialised to 0\n", i);
rt_rq->numInBucket[i]=0;
}
#endif


int main()
{
rt_rq *ourRQ;
init_rt_rq(ourRQ);


}