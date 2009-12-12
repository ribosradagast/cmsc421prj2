/*
* BRR Scheduling Class (mapped to the SCHED_BRR 
* policies)
*/

//method header
static int getNextBucketNumber(struct rt_rq *rt_rq, int currentBucket);
static void enqueue_rt_entity_brr(struct sched_rt_entity *rt_se);
static void dequeue_rt_entity_brr(struct sched_rt_entity *rt_se);



static struct task_struct *_pick_next_task_rt_brr(struct rq *rq)
{
struct sched_rt_entity *rt_se;
struct task_struct *p;
struct rt_rq *rt_rq;
	
int num = -42;
	
	
rt_rq = &rq->rt;

if (unlikely(!rt_rq->rt_nr_running))
return NULL;

if (rt_rq_throttled(rt_rq))
return NULL;



//get the currently-running bucket number
num = getNextBucketNumber(rt_rq,   rq->curr->bid);

do{
	do {
		rt_se = pick_next_rt_entity(rq, rt_rq);
		BUG_ON(!rt_se);
		rt_rq = group_rt_rq(rt_se);
	} while (rt_rq);

	p = rt_task_of(rt_se);
		
	printk("Bucket id for the one that we've chosen is: %d\n", p->bid);
	}while(p->bid!=num);
	
	
	/*
	DONE
start at the position of the current bucket
which we thankfully know, because it's stored in the currently-running
task (p)
Then we want to find the next-highest bucket (or the first-lowest) bucket
(using mod)
then we return the index of this last bucket
*/



p = rt_task_of(rt_se);
p->se.exec_start = rq->clock;

return p;
}

static struct task_struct *pick_next_task_rt_brr(struct rq *rq)
{
struct task_struct *p = _pick_next_task_rt_brr(rq);

/* The running task is never eligible for pushing */
if (p)
dequeue_pushable_task(rq, p);

return p;
}




static void sched_rt_rq_enqueue_brr(struct rt_rq *rt_rq)
{
	struct task_struct *curr = rq_of_rt_rq(rt_rq)->curr;
	struct sched_rt_entity *rt_se = rt_rq->rt_se;

	if (rt_rq->rt_nr_running) {
		if (rt_se && !on_rt_rq(rt_se))
		enqueue_rt_entity_brr(rt_se);
		if (rt_rq->highest_prio.curr < curr->prio)
		resched_task(curr);
	}
}

static void sched_rt_rq_dequeue_brr(struct rt_rq *rt_rq)
{
	struct sched_rt_entity *rt_se = rt_rq->rt_se;

	if (rt_se && on_rt_rq(rt_se))
	dequeue_rt_entity_brr(rt_se);
}

static void __enqueue_rt_entity_brr(struct sched_rt_entity *rt_se)
{
	struct rt_rq *rt_rq = rt_rq_of_se(rt_se);
	struct rt_prio_array *array = &rt_rq->active;
	struct rt_rq *group_rq = group_rt_rq(rt_se);
	struct list_head *queue = array->queue + rt_se_prio(rt_se);
	struct task_struct *p = rt_task_of(rt_se);


	/*
this is where our 50 lines go
if p=>bid==-1, search through bitmap of "taken" slots ie 
int bitmap = int[maxintvalue];
until we find a 0, then set p=>bid to i
set bitmap[i]=1

DONE
Check for if (rt_task_of(rt_se)=>bid == -1)
if so, loop through available buckets and determine 
*/
	int i=0;
	int bucketToAddTo = p->bid;

	if(bucketToAddTo==-1){
		for (i = 0; i < MAX_BRR_PRIO; i++) {
			if(rt_rq->numInBucket[i]==0){
				bucketToAddTo=i;
				break;
			}
		}
	}
	rt_rq->numInBucket[bucketToAddTo]++;
	p->bid=bucketToAddTo;
	

	/*
* Don't enqueue the group if its throttled, or when empty.
* The latter is a consequence of the former when a child group
* get throttled and the current group doesn't have any other
* active members.
*/
	if (group_rq && (rt_rq_throttled(group_rq) || !group_rq->rt_nr_running))
	return;

	list_add_tail(&rt_se->run_list, queue);
	__set_bit(rt_se_prio(rt_se), array->bitmap);

	inc_rt_tasks(rt_se, rt_rq);
}





static void __dequeue_rt_entity_brr(struct sched_rt_entity *rt_se)
{
	struct rt_rq *rt_rq = rt_rq_of_se(rt_se);
	struct rt_prio_array *array = &rt_rq->active;



	/*

DONE: First, remove 1 from the count of bucketarray

compare total running in rq

*/
	int i=0;
	int count=0;
	int bucketToAddTo=rt_task_of(rt_se)->bid;
	rt_rq->numInBucket[bucketToAddTo]--;
	
	for (i = 0; i < MAX_BRR_PRIO; i++) {
		count+=rt_rq->numInBucket[bucketToAddTo];
	}
	printk("Number in queue is: %lu\n", rt_rq->rt_nr_running);
	printk("Number counted from array is: %d\n", count);
	




	list_del_init(&rt_se->run_list);
	if (list_empty(array->queue + rt_se_prio(rt_se)))
	__clear_bit(rt_se_prio(rt_se), array->bitmap);

	dec_rt_tasks(rt_se, rt_rq);
}

/*
* Because the prio of an upper entry depends on the lower
* entries, we must remove entries top - down.
*/
static void dequeue_rt_stack_brr(struct sched_rt_entity *rt_se)
{
	struct sched_rt_entity *back = NULL;


	for_each_sched_rt_entity(rt_se) {
		rt_se->back = back;
		back = rt_se;
	}

	for (rt_se = back; rt_se; rt_se = rt_se->back) {
		if (on_rt_rq(rt_se))
		__dequeue_rt_entity_brr(rt_se);
	}
}

static void enqueue_rt_entity_brr(struct sched_rt_entity *rt_se)
{
	dequeue_rt_stack_brr(rt_se);
	for_each_sched_rt_entity(rt_se)
	__enqueue_rt_entity_brr(rt_se);
}

static void dequeue_rt_entity_brr(struct sched_rt_entity *rt_se)
{
	dequeue_rt_stack_brr(rt_se);

	for_each_sched_rt_entity(rt_se) {
		struct rt_rq *rt_rq = group_rt_rq(rt_se);

		if (rt_rq && rt_rq->rt_nr_running)
		__enqueue_rt_entity_brr(rt_se);
	}
}

/*
* Adding/removing a task to/from a priority array:
*/
static void enqueue_task_rt_brr(struct rq *rq, struct task_struct *p, int wakeup)
{
	struct sched_rt_entity *rt_se = &p->rt;

	if (wakeup)
	rt_se->timeout = 0;

	enqueue_rt_entity_brr(rt_se);

	if (!task_current(rq, p) && p->rt.nr_cpus_allowed > 1)
	enqueue_pushable_task(rq, p);

	inc_cpu_load(rq, p->se.load.weight);
}


static int sched_rt_runtime_exceeded_brr(struct rt_rq *rt_rq)
{
u64 runtime = sched_rt_runtime(rt_rq);

if (rt_rq->rt_throttled)
return rt_rq_throttled(rt_rq);

if (sched_rt_runtime(rt_rq) >= sched_rt_period(rt_rq))
return 0;

balance_runtime(rt_rq);
runtime = sched_rt_runtime(rt_rq);
if (runtime == RUNTIME_INF)
return 0;

if (rt_rq->rt_time > runtime) {
rt_rq->rt_throttled = 1;
if (rt_rq_throttled(rt_rq)) {
sched_rt_rq_dequeue_brr(rt_rq);
return 1;
}
}

return 0;
}



/*
* Update the current task's runtime statistics. Skip current tasks that
* are not in our scheduling class.
*/
static void update_curr_rt_brr(struct rq *rq)
{
struct task_struct *curr = rq->curr;
struct sched_rt_entity *rt_se = &curr->rt;
struct rt_rq *rt_rq = rt_rq_of_se(rt_se);
u64 delta_exec;

if (!task_has_rt_policy(curr))
return;

delta_exec = rq->clock - curr->se.exec_start;
if (unlikely((s64)delta_exec < 0))
delta_exec = 0;

schedstat_set(curr->se.exec_max, max(curr->se.exec_max, delta_exec));

curr->se.sum_exec_runtime += delta_exec;
account_group_exec_runtime(curr, delta_exec);

curr->se.exec_start = rq->clock;
cpuacct_charge(curr, delta_exec);

if (!rt_bandwidth_enabled())
return;

for_each_sched_rt_entity(rt_se) {
rt_rq = rt_rq_of_se(rt_se);

if (sched_rt_runtime(rt_rq) != RUNTIME_INF) {
spin_lock(&rt_rq->rt_runtime_lock);
rt_rq->rt_time += delta_exec;
if (sched_rt_runtime_exceeded_brr(rt_rq))
resched_task(curr);
spin_unlock(&rt_rq->rt_runtime_lock);
}
}
}


static void dequeue_task_rt_brr(struct rq *rq, struct task_struct *p, int sleep)
{
	struct sched_rt_entity *rt_se = &p->rt;

	update_curr_rt_brr(rq);
	dequeue_rt_entity_brr(rt_se);

	dequeue_pushable_task(rq, p);

	dec_cpu_load(rq, p->se.load.weight);
}

/*
DONE: get the bucket number that we want to assign the next task to
*/
static  int getNextBucketNumber(struct rt_rq *rt_rq, int currentBucket)
{
	int offset=0;
	printk("WHOOOOOOO getNextBucketNumber was called!\n");
	for(offset=currentBucket+1; offset<=MAX_BRR_PRIO+1;offset++){
		if(rt_rq->numInBucket[offset % MAX_BRR_PRIO]!=0){
			return offset;
		}
	}
	return -1;
}

static int do_sched_rt_period_timer_brr(struct rt_bandwidth *rt_b, int overrun)
{
int i, idle = 1;
const struct cpumask *span;

if (!rt_bandwidth_enabled() || rt_b->rt_runtime == RUNTIME_INF)
return 1;

span = sched_rt_period_mask();
for_each_cpu(i, span) {
int enqueue = 0;
struct rt_rq *rt_rq = sched_rt_period_rt_rq(rt_b, i);
struct rq *rq = rq_of_rt_rq(rt_rq);

spin_lock(&rq->lock);
if (rt_rq->rt_time) {
u64 runtime;

spin_lock(&rt_rq->rt_runtime_lock);
if (rt_rq->rt_throttled)
balance_runtime(rt_rq);
runtime = rt_rq->rt_runtime;
rt_rq->rt_time -= min(rt_rq->rt_time, overrun*runtime);
if (rt_rq->rt_throttled && rt_rq->rt_time < runtime) {
rt_rq->rt_throttled = 0;
enqueue = 1;
}
if (rt_rq->rt_time || rt_rq->rt_nr_running)
idle = 0;
spin_unlock(&rt_rq->rt_runtime_lock);
} else if (rt_rq->rt_nr_running)
idle = 0;

if (enqueue)
sched_rt_rq_enqueue_brr(rt_rq);
spin_unlock(&rq->lock);
}

return idle;
}

static void put_prev_task_rt_brr(struct rq *rq, struct task_struct *p)
{
update_curr_rt_brr(rq);
p->se.exec_start = 0;

/*
* The previous task needs to be made eligible for pushing
* if it is still active
*/
if (p->se.on_rq && p->rt.nr_cpus_allowed > 1)
enqueue_pushable_task(rq, p);
}



static void task_tick_rt_brr(struct rq *rq, struct task_struct *p, int queued)
{
update_curr_rt_brr(rq);

watchdog(rq, p);

/*
* RR tasks need a special form of timeslice management.
* FIFO tasks have no timeslices.
*/
if (p->policy != SCHED_RR)
return;

if (--p->rt.time_slice)
return;

p->rt.time_slice = DEF_TIMESLICE;

/*
* Requeue to the end of queue if we are not the only element
* on the queue:
*/
if (p->rt.run_list.prev != p->rt.run_list.next) {
requeue_task_rt(rq, p, 0);
set_tsk_need_resched(p);
}
}




static const struct sched_class brr_sched_class = {
	.next = &fair_sched_class,
	.enqueue_task = enqueue_task_rt_brr,
	.dequeue_task = dequeue_task_rt_brr,
	.yield_task = yield_task_rt,

	.check_preempt_curr = check_preempt_curr_rt,

	.pick_next_task = pick_next_task_rt_brr,
	.put_prev_task = put_prev_task_rt_brr,

#ifdef CONFIG_SMP
	.select_task_rq = select_task_rq_rt,

	.load_balance = load_balance_rt,
	.move_one_task = move_one_task_rt,
	.set_cpus_allowed       = set_cpus_allowed_rt,
	.rq_online              = rq_online_rt,
	.rq_offline             = rq_offline_rt,
	.pre_schedule = pre_schedule_rt,
	.needs_post_schedule = needs_post_schedule_rt,
	.post_schedule = post_schedule_rt,
	.task_wake_up = task_wake_up_rt,
	.switched_from = switched_from_rt,
#endif

	.set_curr_task          = set_curr_task_rt,
	.task_tick = task_tick_rt_brr,

	.prio_changed = prio_changed_rt,
	.switched_to = switched_to_rt,
};



static const struct sched_class rt_sched_class = {
.next = &brr_sched_class,
.enqueue_task = enqueue_task_rt,
.dequeue_task = dequeue_task_rt,
.yield_task = yield_task_rt,

.check_preempt_curr = check_preempt_curr_rt,

.pick_next_task = pick_next_task_rt,
.put_prev_task = put_prev_task_rt,

#ifdef CONFIG_SMP
.select_task_rq = select_task_rq_rt,

.load_balance = load_balance_rt,
.move_one_task = move_one_task_rt,
.set_cpus_allowed       = set_cpus_allowed_rt,
.rq_online              = rq_online_rt,
.rq_offline             = rq_offline_rt,
.pre_schedule = pre_schedule_rt,
.needs_post_schedule = needs_post_schedule_rt,
.post_schedule = post_schedule_rt,
.task_wake_up = task_wake_up_rt,
.switched_from = switched_from_rt,
#endif

.set_curr_task          = set_curr_task_rt,
.task_tick = task_tick_rt,

.prio_changed = prio_changed_rt,
.switched_to = switched_to_rt,
};


#ifdef CONFIG_SCHED_DEBUG
extern void print_rt_rq(struct seq_file *m, int cpu, struct rt_rq *rt_rq);

static void print_rt_stats(struct seq_file *m, int cpu)
{
	struct rt_rq *rt_rq;

	rcu_read_lock();
	for_each_leaf_rt_rq(rt_rq, cpu_rq(cpu))
	print_rt_rq(m, cpu, rt_rq);
	rcu_read_unlock();
}
#endif /* CONFIG_SCHED_DEBUG */

