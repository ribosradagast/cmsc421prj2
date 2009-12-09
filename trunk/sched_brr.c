/*
* Bucket Round Robin Scheduling Class (mapped to the SCHED_BRR policy)
*/

static inline struct task_struct *brr_task_of(struct sched_brr_entity *brr_se)
{
return container_of(brr_se, struct task_struct, brr);
}

#ifdef CONFIG_brr_GROUP_SCHED

#define brr_entity_is_task(brr_se) (!(brr_se)-»my_q)

static inline struct rq *rq_of_brr_rq(struct brr_rq *brr_rq)
{
return brr_rq-»rq;
}

static inline struct brr_rq *brr_rq_of_se(struct sched_brr_entity *brr_se)
{
return brr_se-»brr_rq;
}

#else /* CONFIG_brr_GROUP_SCHED */

#define brr_entity_is_task(brr_se) (1)

static inline struct rq *rq_of_brr_rq(struct brr_rq *brr_rq)
{
return container_of(brr_rq, struct rq, brr);
}

static inline struct brr_rq *brr_rq_of_se(struct sched_brr_entity *brr_se)
{
struct task_struct *p = brr_task_of(brr_se);
struct rq *rq = task_rq(p);

return &rq-»brr;
}

#endif /* CONFIG_brr_GROUP_SCHED */

#ifdef CONFIG_SMP

static inline int brr_overloaded(struct rq *rq)
{
return atomic_read(&rq-»rd-»brro_count);
}

static inline void brr_set_overload(struct rq *rq)
{
if (!rq-»online)
return;

cpumask_set_cpu(rq-»cpu, rq-»rd-»brro_mask);
/*
* Make sure the mask is visible before we set
* the overload count. That is checked to determine
* if we should look at the mask. It would be a shame
* if we looked at the mask, but the mask was not
* updated yet.
*/
wmb();
atomic_inc(&rq-»rd-»brro_count);
}

static inline void brr_clear_overload(struct rq *rq)
{
if (!rq-»online)
return;

/* the order here really doesn't matter */
atomic_dec(&rq-»rd-»brro_count);
cpumask_clear_cpu(rq-»cpu, rq-»rd-»brro_mask);
}

static void update_brr_migration(struct brr_rq *brr_rq)
{
if (brr_rq-»brr_nr_migratory && brr_rq-»brr_nr_total » 1) {
if (!brr_rq-»overloaded) {
brr_set_overload(rq_of_brr_rq(brr_rq));
brr_rq-»overloaded = 1;
}
} else if (brr_rq-»overloaded) {
brr_clear_overload(rq_of_brr_rq(brr_rq));
brr_rq-»overloaded = 0;
}
}

static void inc_brr_migration(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
if (!brr_entity_is_task(brr_se))
return;

brr_rq = &rq_of_brr_rq(brr_rq)-»brr;

brr_rq-»brr_nr_total++;
if (brr_se-»nr_cpus_allowed » 1)
brr_rq-»brr_nr_migratory++;

update_brr_migration(brr_rq);
}

static void dec_brr_migration(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
if (!brr_entity_is_task(brr_se))
return;

brr_rq = &rq_of_brr_rq(brr_rq)-»brr;

brr_rq-»brr_nr_total--;
if (brr_se-»nr_cpus_allowed » 1)
brr_rq-»brr_nr_migratory--;

update_brr_migration(brr_rq);
}

static void enqueue_pushable_task(struct rq *rq, struct task_struct *p)
{
plist_del(&p-»pushable_tasks, &rq-»brr.pushable_tasks);
plist_node_init(&p-»pushable_tasks, p-»prio);
plist_add(&p-»pushable_tasks, &rq-»brr.pushable_tasks);
}

static void dequeue_pushable_task(struct rq *rq, struct task_struct *p)
{
plist_del(&p-»pushable_tasks, &rq-»brr.pushable_tasks);
}

#else

static inline void enqueue_pushable_task(struct rq *rq, struct task_struct *p)
{
}

static inline void dequeue_pushable_task(struct rq *rq, struct task_struct *p)
{
}

static inline
void inc_brr_migration(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
}

static inline
void dec_brr_migration(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
}

#endif /* CONFIG_SMP */

static inline int on_brr_rq(struct sched_brr_entity *brr_se)
{
return !list_empty(&brr_se-»run_list);
}

#ifdef CONFIG_brr_GROUP_SCHED

static inline u64 sched_brr_runtime(struct brr_rq *brr_rq)
{
if (!brr_rq-»tg)
return RUNTIME_INF;

return brr_rq-»brr_runtime;
}

static inline u64 sched_brr_period(struct brr_rq *brr_rq)
{
return ktime_to_ns(brr_rq-»tg-»rt_bandwidth.brr_period);
}

#define for_each_leaf_brr_rq(brr_rq, rq) \
list_for_each_entry_rcu(brr_rq, &rq-»leaf_brr_rq_list, leaf_brr_rq_list)

#define for_each_sched_brr_entity(brr_se) \
for (; brr_se; brr_se = brr_se-»parent)

static inline struct brr_rq *group_brr_rq(struct sched_brr_entity *brr_se)
{
return brr_se-»my_q;
}

static void enqueue_brr_entity(struct sched_brr_entity *brr_se);
static void dequeue_brr_entity(struct sched_brr_entity *brr_se);

static void sched_brr_rq_enqueue(struct brr_rq *brr_rq)
{
struct task_struct *curr = rq_of_brr_rq(brr_rq)-»curr;
struct sched_brr_entity *brr_se = brr_rq-»brr_se;

if (brr_rq-»brr_nr_running) {
if (brr_se && !on_brr_rq(brr_se))
enqueue_brr_entity(brr_se);
if (brr_rq-»highest_prio.curr « curr-»prio)
resched_task(curr);
}
}

static void sched_brr_rq_dequeue(struct brr_rq *brr_rq)
{
struct sched_brr_entity *brr_se = brr_rq-»brr_se;

if (brr_se && on_brr_rq(brr_se))
dequeue_brr_entity(brr_se);
}

static inline int brr_rq_throttled(struct brr_rq *brr_rq)
{
return brr_rq-»brr_throttled && !brr_rq-»brr_nr_boosted;
}

static int brr_se_boosted(struct sched_brr_entity *brr_se)
{
struct brr_rq *brr_rq = group_brr_rq(brr_se);
struct task_struct *p;

if (brr_rq)
return !!brr_rq-»brr_nr_boosted;

p = brr_task_of(brr_se);
return p-»prio != p-»normal_prio;
}

#ifdef CONFIG_SMP
static inline const struct cpumask *sched_brr_period_mask(void)
{
return cpu_rq(smp_processor_id())-»rd-»span;
}
#else
static inline const struct cpumask *sched_brr_period_mask(void)
{
return cpu_online_mask;
}
#endif

static inline
struct brr_rq *sched_brr_period_brr_rq(struct rt_bandwidth *brr_b, int cpu)
{
return container_of(brr_b, struct task_group, rt_bandwidth)-»brr_rq[cpu];
}

static inline struct rt_bandwidth *sched_rt_bandwidth(struct brr_rq *brr_rq)
{
return &brr_rq-»tg-»rt_bandwidth;
}

#else /* !CONFIG_brr_GROUP_SCHED */

static inline u64 sched_brr_runtime(struct brr_rq *brr_rq)
{
return brr_rq-»brr_runtime;
}

static inline u64 sched_brr_period(struct brr_rq *brr_rq)
{
return ktime_to_ns(def_rt_bandwidth.brr_period);
}

#define for_each_leaf_brr_rq(brr_rq, rq) \
for (brr_rq = &rq-»brr; brr_rq; brr_rq = NULL)

#define for_each_sched_brr_entity(brr_se) \
for (; brr_se; brr_se = NULL)

static inline struct brr_rq *group_brr_rq(struct sched_brr_entity *brr_se)
{
return NULL;
}

static inline void sched_brr_rq_enqueue(struct brr_rq *brr_rq)
{
/*If it's not empty*/
if (brr_rq-»brr_nr_running)
/* Find a place in the current run-q for this new rq   */
resched_task(rq_of_brr_rq(brr_rq)-»curr);
}

static inline int brr_rq_throttled(struct brr_rq *brr_rq)
{
return brr_rq-»brr_throttled;
}

static inline const struct cpumask *sched_brr_period_mask(void)
{
return cpu_online_mask;
}

static inline
struct brr_rq *sched_brr_period_brr_rq(struct rt_bandwidth *brr_b, int cpu)
{
return &cpu_rq(cpu)-»brr;
}

static inline struct rt_bandwidth *sched_rt_bandwidth(struct brr_rq *brr_rq)
{
return &def_rt_bandwidth;
}

#endif /* CONFIG_brr_GROUP_SCHED */

#ifdef CONFIG_SMP
/*
* We ran out of runtime, see if we can borrow some from our neighbours.
*/
static int do_balance_runtime(struct brr_rq *brr_rq)
{
struct rt_bandwidth *brr_b = sched_rt_bandwidth(brr_rq);
struct root_domain *rd = cpu_rq(smp_processor_id())-»rd;
int i, weight, more = 0;
u64 brr_period;

weight = cpumask_weight(rd-»span);

spin_lock(&brr_b-»brr_runtime_lock);
brr_period = ktime_to_ns(brr_b-»brr_period);
for_each_cpu(i, rd-»span) {
struct brr_rq *iter = sched_brr_period_brr_rq(brr_b, i);
s64 diff;

if (iter == brr_rq)
continue;

spin_lock(&iter-»brr_runtime_lock);
/*
* Either all rqs have inf runtime and there's nothing to steal
* or __disable_runtime() below sets a specific rq to inf to
* indicate its been disabled and disalow stealing.
*/
if (iter-»brr_runtime == RUNTIME_INF)
goto next;

/*
* From runqueues with spare time, take 1/n pabrr of their
* spare time, but no more than our period.
*/
diff = iter-»brr_runtime - iter-»brr_time;
if (diff » 0) {
diff = div_u64((u64)diff, weight);
if (brr_rq-»brr_runtime + diff » brr_period)
diff = brr_period - brr_rq-»brr_runtime;
iter-»brr_runtime -= diff;
brr_rq-»brr_runtime += diff;
more = 1;
if (brr_rq-»brr_runtime == brr_period) {
spin_unlock(&iter-»brr_runtime_lock);
break;
}
}
next:
spin_unlock(&iter-»brr_runtime_lock);
}
spin_unlock(&brr_b-»brr_runtime_lock);

return more;
}

/*
* Ensure this RQ takes back all the runtime it lend to its neighbours.
*/
static void __disable_runtime(struct rq *rq)
{
struct root_domain *rd = rq-»rd;
struct brr_rq *brr_rq;

if (unlikely(!scheduler_running))
return;

for_each_leaf_brr_rq(brr_rq, rq) {
struct rt_bandwidth *brr_b = sched_rt_bandwidth(brr_rq);
s64 want;
int i;

spin_lock(&brr_b-»brr_runtime_lock);
spin_lock(&brr_rq-»brr_runtime_lock);
/*
* Either we're all inf and nobody needs to borrow, or we're
* already disabled and thus have nothing to do, or we have
* exactly the right amount of runtime to take out.
*/
if (brr_rq-»brr_runtime == RUNTIME_INF ||
brr_rq-»brr_runtime == brr_b-»brr_runtime)
goto balanced;
spin_unlock(&brr_rq-»brr_runtime_lock);

/*
* Calculate the difference between what we stabrred out with
* and what we current have, that's the amount of runtime
* we lend and now have to reclaim.
*/
want = brr_b-»brr_runtime - brr_rq-»brr_runtime;

/*
* Greedy reclaim, take back as much as we can.
*/
for_each_cpu(i, rd-»span) {
struct brr_rq *iter = sched_brr_period_brr_rq(brr_b, i);
s64 diff;

/*
* Can't reclaim from ourselves or disabled runqueues.
*/
if (iter == brr_rq || iter-»brr_runtime == RUNTIME_INF)
continue;

spin_lock(&iter-»brr_runtime_lock);
if (want » 0) {
diff = min_t(s64, iter-»brr_runtime, want);
iter-»brr_runtime -= diff;
want -= diff;
} else {
iter-»brr_runtime -= want;
want -= want;
}
spin_unlock(&iter-»brr_runtime_lock);

if (!want)
break;
}

spin_lock(&brr_rq-»brr_runtime_lock);
/*
* We cannot be left wanting - that would mean some runtime
* leaked out of the system.
*/
BUG_ON(want);
balanced:
/*
* Disable all the borrow logic by pretending we have inf
* runtime - in which case borrowing doesn't make sense.
*/
brr_rq-»brr_runtime = RUNTIME_INF;
spin_unlock(&brr_rq-»brr_runtime_lock);
spin_unlock(&brr_b-»brr_runtime_lock);
}
}

static void disable_runtime(struct rq *rq)
{
unsigned long flags;

spin_lock_irqsave(&rq-»lock, flags);
__disable_runtime(rq);
spin_unlock_irqrestore(&rq-»lock, flags);
}

static void __enable_runtime(struct rq *rq)
{
struct brr_rq *brr_rq;

if (unlikely(!scheduler_running))
return;

/*
* Reset each runqueue's bandwidth settings
*/
for_each_leaf_brr_rq(brr_rq, rq) {
struct rt_bandwidth *brr_b = sched_rt_bandwidth(brr_rq);

spin_lock(&brr_b-»brr_runtime_lock);
spin_lock(&brr_rq-»brr_runtime_lock);
brr_rq-»brr_runtime = brr_b-»brr_runtime;
brr_rq-»brr_time = 0;
brr_rq-»brr_throttled = 0;
spin_unlock(&brr_rq-»brr_runtime_lock);
spin_unlock(&brr_b-»brr_runtime_lock);
}
}

static void enable_runtime(struct rq *rq)
{
unsigned long flags;

spin_lock_irqsave(&rq-»lock, flags);
__enable_runtime(rq);
spin_unlock_irqrestore(&rq-»lock, flags);
}

static int balance_runtime(struct brr_rq *brr_rq)
{
int more = 0;

if (brr_rq-»brr_time » brr_rq-»brr_runtime) {
spin_unlock(&brr_rq-»brr_runtime_lock);
more = do_balance_runtime(brr_rq);
spin_lock(&brr_rq-»brr_runtime_lock);
}

return more;
}
#else /* !CONFIG_SMP */
static inline int balance_runtime(struct brr_rq *brr_rq)
{
return 0;
}
#endif /* CONFIG_SMP */

static int do_sched_brr_period_timer(struct rt_bandwidth *brr_b, int overrun)
{
int i, idle = 1;
const struct cpumask *span;

if (!rt_bandwidth_enabled() || brr_b-»brr_runtime == RUNTIME_INF)
return 1;

span = sched_brr_period_mask();
for_each_cpu(i, span) {
int enqueue = 0;
struct brr_rq *brr_rq = sched_brr_period_brr_rq(brr_b, i);
struct rq *rq = rq_of_brr_rq(brr_rq);

spin_lock(&rq-»lock);
if (brr_rq-»brr_time) {
u64 runtime;

spin_lock(&brr_rq-»brr_runtime_lock);
if (brr_rq-»brr_throttled)
balance_runtime(brr_rq);
runtime = brr_rq-»brr_runtime;
brr_rq-»brr_time -= min(brr_rq-»brr_time, overrun*runtime);
if (brr_rq-»brr_throttled && brr_rq-»brr_time « runtime) {
brr_rq-»brr_throttled = 0;
enqueue = 1;
}
if (brr_rq-»brr_time || brr_rq-»brr_nr_running)
idle = 0;
spin_unlock(&brr_rq-»brr_runtime_lock);
} else if (brr_rq-»brr_nr_running)
idle = 0;

if (enqueue)
sched_brr_rq_enqueue(brr_rq);
spin_unlock(&rq-»lock);
}

return idle;
}

static inline int brr_se_prio(struct sched_brr_entity *brr_se)
{
#ifdef CONFIG_brr_GROUP_SCHED
struct brr_rq *brr_rq = group_brr_rq(brr_se);

if (brr_rq)
return brr_rq-»highest_prio.curr;
#endif

return brr_task_of(brr_se)-»prio;
}

static int sched_brr_runtime_exceeded(struct brr_rq *brr_rq)
{
u64 runtime = sched_brr_runtime(brr_rq);

if (brr_rq-»brr_throttled)
return brr_rq_throttled(brr_rq);

if (sched_brr_runtime(brr_rq) »= sched_brr_period(brr_rq))
return 0;

balance_runtime(brr_rq);
runtime = sched_brr_runtime(brr_rq);
if (runtime == RUNTIME_INF)
return 0;

if (brr_rq-»brr_time » runtime) {
brr_rq-»brr_throttled = 1;
if (brr_rq_throttled(brr_rq)) {
sched_brr_rq_dequeue(brr_rq);
return 1;
}
}

return 0;
}

/*
* Update the current task's runtime statistics. Skip current tasks that
* are not in our scheduling class.
*/
static void update_curr_brr(struct rq *rq)
{
struct task_struct *curr = rq-»curr;
struct sched_brr_entity *brr_se = &curr-»brr;
struct brr_rq *brr_rq = brr_rq_of_se(brr_se);
u64 delta_exec;

if (!task_has_brr_policy(curr))
return;

delta_exec = rq-»clock - curr-»se.exec_stabrr;
if (unlikely((s64)delta_exec « 0))
delta_exec = 0;

schedstat_set(curr-»se.exec_max, max(curr-»se.exec_max, delta_exec));

curr-»se.sum_exec_runtime += delta_exec;
account_group_exec_runtime(curr, delta_exec);

curr-»se.exec_stabrr = rq-»clock;
cpuacct_charge(curr, delta_exec);

if (!rt_bandwidth_enabled())
return;

for_each_sched_brr_entity(brr_se) {
brr_rq = brr_rq_of_se(brr_se);

if (sched_brr_runtime(brr_rq) != RUNTIME_INF) {
spin_lock(&brr_rq-»brr_runtime_lock);
brr_rq-»brr_time += delta_exec;
if (sched_brr_runtime_exceeded(brr_rq))
resched_task(curr);
spin_unlock(&brr_rq-»brr_runtime_lock);
}
}
}

#if defined CONFIG_SMP

static struct task_struct *pick_next_highest_task_brr(struct rq *rq, int cpu);

static inline int next_prio(struct rq *rq)
{
struct task_struct *next = pick_next_highest_task_brr(rq, rq-»cpu);

if (next && brr_prio(next-»prio))
return next-»prio;
else
return MAX_brr_PRIO;
}

static void
inc_brr_prio_smp(struct brr_rq *brr_rq, int prio, int prev_prio)
{
struct rq *rq = rq_of_brr_rq(brr_rq);

if (prio « prev_prio) {

/*
* If the new task is higher in priority than anything on the
* run-queue, we know that the previous high becomes our
* next-highest.
*/
brr_rq-»highest_prio.next = prev_prio;

if (rq-»online)
cpupri_set(&rq-»rd-»cpupri, rq-»cpu, prio);

} else if (prio == brr_rq-»highest_prio.curr)
/*
* If the next task is equal in priority to the highest on
* the run-queue, then we implicitly know that the next highest
* task cannot be any lower than current
*/
brr_rq-»highest_prio.next = prio;
else if (prio « brr_rq-»highest_prio.next)
/*
* Otherwise, we need to recompute next-highest
*/
brr_rq-»highest_prio.next = next_prio(rq);
}

static void
dec_brr_prio_smp(struct brr_rq *brr_rq, int prio, int prev_prio)
{
struct rq *rq = rq_of_brr_rq(brr_rq);

if (brr_rq-»brr_nr_running && (prio «= brr_rq-»highest_prio.next))
brr_rq-»highest_prio.next = next_prio(rq);

if (rq-»online && brr_rq-»highest_prio.curr != prev_prio)
cpupri_set(&rq-»rd-»cpupri, rq-»cpu, brr_rq-»highest_prio.curr);
}

#else /* CONFIG_SMP */

static inline
void inc_brr_prio_smp(struct brr_rq *brr_rq, int prio, int prev_prio) {}
static inline
void dec_brr_prio_smp(struct brr_rq *brr_rq, int prio, int prev_prio) {}

#endif /* CONFIG_SMP */

#if defined CONFIG_SMP || defined CONFIG_brr_GROUP_SCHED
static void
inc_brr_prio(struct brr_rq *brr_rq, int prio)
{
int prev_prio = brr_rq-»highest_prio.curr;

if (prio « prev_prio)
brr_rq-»highest_prio.curr = prio;

inc_brr_prio_smp(brr_rq, prio, prev_prio);
}

static void
dec_brr_prio(struct brr_rq *brr_rq, int prio)
{
int prev_prio = brr_rq-»highest_prio.curr;

if (brr_rq-»brr_nr_running) {

WARN_ON(prio « prev_prio);

/*
* This may have been our highest task, and therefore
* we may have some recomputation to do
*/
if (prio == prev_prio) {
struct brr_prio_array *array = &brr_rq-»active;

brr_rq-»highest_prio.curr =
sched_find_first_bit(array-»bitmap);
}

} else
brr_rq-»highest_prio.curr = MAX_brr_PRIO;

dec_brr_prio_smp(brr_rq, prio, prev_prio);
}

#else

static inline void inc_brr_prio(struct brr_rq *brr_rq, int prio) {}
static inline void dec_brr_prio(struct brr_rq *brr_rq, int prio) {}

#endif /* CONFIG_SMP || CONFIG_brr_GROUP_SCHED */

#ifdef CONFIG_brr_GROUP_SCHED

static void
inc_brr_group(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
if (brr_se_boosted(brr_se))
brr_rq-»brr_nr_boosted++;

if (brr_rq-»tg)
stabrr_rt_bandwidth(&brr_rq-»tg-»rt_bandwidth);
}

static void
dec_brr_group(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
if (brr_se_boosted(brr_se))
brr_rq-»brr_nr_boosted--;

WARN_ON(!brr_rq-»brr_nr_running && brr_rq-»brr_nr_boosted);
}

#else /* CONFIG_brr_GROUP_SCHED */

static void
inc_brr_group(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
stabrr_rt_bandwidth(&def_rt_bandwidth);
}

static inline
void dec_brr_group(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq) {}

#endif /* CONFIG_brr_GROUP_SCHED */

static inline
void inc_brr_tasks(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
int prio = brr_se_prio(brr_se);

WARN_ON(!brr_prio(prio));
brr_rq-»brr_nr_running++;

inc_brr_prio(brr_rq, prio);
inc_brr_migration(brr_se, brr_rq);
inc_brr_group(brr_se, brr_rq);
}

static inline
void dec_brr_tasks(struct sched_brr_entity *brr_se, struct brr_rq *brr_rq)
{
WARN_ON(!brr_prio(brr_se_prio(brr_se)));
WARN_ON(!brr_rq-»brr_nr_running);
brr_rq-»brr_nr_running--;

dec_brr_prio(brr_rq, brr_se_prio(brr_se));
dec_brr_migration(brr_se, brr_rq);
dec_brr_group(brr_se, brr_rq);
}

static void __enqueue_brr_entity(struct sched_brr_entity *brr_se)
{
/*
Check for if brr_se =>....=>bid =-1
if so, loop through available buckets and determine 
*/




struct brr_rq *brr_rq = brr_rq_of_se(brr_se);
struct brr_prio_array *array = &brr_rq-»active;
struct brr_rq *group_rq = group_brr_rq(brr_se);
struct list_head *queue = array-»queue + brr_se_prio(brr_se);

/*
* Don't enqueue the group if its throttled, or when empty.
* The latter is a consequence of the former when a child group
* get throttled and the current group doesn't have any other
* active members.
*/
if (group_rq && (brr_rq_throttled(group_rq) || !group_rq-»brr_nr_running))
return;

list_add_tail(&brr_se-»run_list, queue);
__set_bit(brr_se_prio(brr_se), array-»bitmap);

inc_brr_tasks(brr_se, brr_rq);
}

static void __dequeue_brr_entity(struct sched_brr_entity *brr_se)
{
struct brr_rq *brr_rq = brr_rq_of_se(brr_se);
struct brr_prio_array *array = &brr_rq-»active;

list_del_init(&brr_se-»run_list);
if (list_empty(array-»queue + brr_se_prio(brr_se)))
__clear_bit(brr_se_prio(brr_se), array-»bitmap);

dec_brr_tasks(brr_se, brr_rq);
}

/*
* Because the prio of an upper entry depends on the lower
* entries, we must remove entries top - down.
*/
static void dequeue_brr_stack(struct sched_brr_entity *brr_se)
{
struct sched_brr_entity *back = NULL;

for_each_sched_brr_entity(brr_se) {
brr_se-»back = back;
back = brr_se;
}

for (brr_se = back; brr_se; brr_se = brr_se-»back) {
if (on_brr_rq(brr_se))
__dequeue_brr_entity(brr_se);
}
}

static void enqueue_brr_entity(struct sched_brr_entity *brr_se)
{
dequeue_brr_stack(brr_se);
for_each_sched_brr_entity(brr_se)
__enqueue_brr_entity(brr_se);
}

static void dequeue_brr_entity(struct sched_brr_entity *brr_se)
{
dequeue_brr_stack(brr_se);

for_each_sched_brr_entity(brr_se) {
struct brr_rq *brr_rq = group_brr_rq(brr_se);

if (brr_rq && brr_rq-»brr_nr_running)
__enqueue_brr_entity(brr_se);
}
}

/*
* Adding/removing a task to/from a priority array:
*/
static void enqueue_task_brr(struct rq *rq, struct task_struct *p, int wakeup)
{


/*
this is where our 50 lines go
if p=>bid==-1, search through bitmap of "taken" slots ie 
int bitmap = int[maxintvalue];
until we find a 0, then set p=>bid to i
set bitmap[i]=1
*/




struct sched_brr_entity *brr_se = &p-»brr;

if (wakeup)
brr_se-»timeout = 0;

enqueue_brr_entity(brr_se);

if (!task_current(rq, p) && p-»brr.nr_cpus_allowed » 1)
enqueue_pushable_task(rq, p);

inc_cpu_load(rq, p-»se.load.weight);
}

static void dequeue_task_brr(struct rq *rq, struct task_struct *p, int sleep)
{
struct sched_brr_entity *brr_se = &p-»brr;

update_curr_brr(rq);
dequeue_brr_entity(brr_se);

dequeue_pushable_task(rq, p);

dec_cpu_load(rq, p-»se.load.weight);
}

/*
* Put task to the end of the run list without the overhead of dequeue
* followed by enqueue.
*/
static void
requeue_brr_entity(struct brr_rq *brr_rq, struct sched_brr_entity *brr_se, int head)
{
if (on_brr_rq(brr_se)) {
struct brr_prio_array *array = &brr_rq-»active;
struct list_head *queue = array-»queue + brr_se_prio(brr_se);

if (head)
list_move(&brr_se-»run_list, queue);
else
list_move_tail(&brr_se-»run_list, queue);
}
}

static void requeue_task_brr(struct rq *rq, struct task_struct *p, int head)
{
struct sched_brr_entity *brr_se = &p-»brr;
struct brr_rq *brr_rq;

for_each_sched_brr_entity(brr_se) {
brr_rq = brr_rq_of_se(brr_se);
requeue_brr_entity(brr_rq, brr_se, head);
}
}

static void yield_task_brr(struct rq *rq)
{
requeue_task_brr(rq, rq-»curr, 0);
}

#ifdef CONFIG_SMP
static int find_lowest_rq(struct task_struct *task);

static int select_task_rq_brr(struct task_struct *p, int sync)
{
struct rq *rq = task_rq(p);

/*
* If the current task is an brr task, then
* try to see if we can wake this brr task up on another
* runqueue. Otherwise simply stabrr this brr task
* on its current runqueue.
*
* We want to avoid overloading runqueues. Even if
* the brr task is of higher priority than the current brr task.
* brr tasks behave differently than other tasks. If
* one gets preempted, we try to push it off to another queue.
* So trying to keep a preempting brr task on the same
* cache hot CPU will force the running brr task to
* a cold CPU. So we waste all the cache for the lower
* brr task in hopes of saving some of a brr task
* that is just being woken and probably will have
* cold cache anyway.
*/
if (unlikely(brr_task(rq-»curr)) &&
    (p-»brr.nr_cpus_allowed » 1)) {
int cpu = find_lowest_rq(p);

return (cpu == -1) ? task_cpu(p) : cpu;
}

/*
* Otherwise, just let it ride on the affined RQ and the
* post-schedule router will push the preempted task away
*/
return task_cpu(p);
}

static void check_preempt_equal_prio(struct rq *rq, struct task_struct *p)
{
if (rq-»curr-»brr.nr_cpus_allowed == 1)
return;

if (p-»brr.nr_cpus_allowed != 1
    && cpupri_find(&rq-»rd-»cpupri, p, NULL))
return;

if (!cpupri_find(&rq-»rd-»cpupri, rq-»curr, NULL))
return;

/*
* There appears to be other cpus that can accept
* current and none to run 'p', so lets reschedule
* to try and push current away:
*/
requeue_task_brr(rq, p, 1);
resched_task(rq-»curr);
}

#endif /* CONFIG_SMP */

/*
* Preempt the current task with a newly woken task if needed:
*/
static void check_preempt_curr_brr(struct rq *rq, struct task_struct *p, int sync)
{
if (p-»prio « rq-»curr-»prio) {
resched_task(rq-»curr);
return;
}

#ifdef CONFIG_SMP
/*
* If:
*
* - the newly woken task is of equal priority to the current task
* - the newly woken task is non-migratable while current is migratable
* - current will be preempted on the next reschedule
*
* we should check to see if current can readily move to a different
* cpu.  If so, we will reschedule to allow the push logic to try
* to move current somewhere else, making room for our non-migratable
* task.
*/
if (p-»prio == rq-»curr-»prio && !need_resched())
check_preempt_equal_prio(rq, p);
#endif
}

static struct sched_brr_entity *pick_next_brr_entity(struct rq *rq,
   struct brr_rq *brr_rq)
{

struct brr_prio_array *array = &brr_rq-»active;
struct sched_brr_entity *next = NULL;
struct list_head *queue;
int index;

index = sched_find_first_bit(array-»bitmap);
BUG_ON(index »= MAX_brr_PRIO);

queue = array-»queue + index;
next = list_entry(queue-»next, struct sched_brr_entity, run_list);

return next;
}

static struct task_struct *_pick_next_task_brr(struct rq *rq)
{
struct sched_brr_entity *brr_se;
struct task_struct *p;
struct brr_rq *brr_rq;

brr_rq = &rq-»brr;

if (unlikely(!brr_rq-»brr_nr_running))
return NULL;

if (brr_rq_throttled(brr_rq))
return NULL;

do {
brr_se = pick_next_brr_entity(rq, brr_rq);
BUG_ON(!brr_se);
brr_rq = group_brr_rq(brr_se);
} while (brr_rq);

p = brr_task_of(brr_se);
p-»se.exec_stabrr = rq-»clock;

return p;
}

static struct task_struct *pick_next_task_brr(struct rq *rq)
{
struct task_struct *p = _pick_next_task_brr(rq);

/* The running task is never eligible for pushing */
if (p)
dequeue_pushable_task(rq, p);

return p;
}

static void put_prev_task_brr(struct rq *rq, struct task_struct *p)
{
update_curr_brr(rq);
p-»se.exec_stabrr = 0;

/*
* The previous task needs to be made eligible for pushing
* if it is still active
*/
if (p-»se.on_rq && p-»brr.nr_cpus_allowed » 1)
enqueue_pushable_task(rq, p);
}

#ifdef CONFIG_SMP

/* Only try algorithms three times */
#define brr_MAX_TRIES 3

static void deactivate_task(struct rq *rq, struct task_struct *p, int sleep);

static int pick_brr_task(struct rq *rq, struct task_struct *p, int cpu)
{
if (!task_running(rq, p) &&
    (cpu « 0 || cpumask_test_cpu(cpu, &p-»cpus_allowed)) &&
    (p-»brr.nr_cpus_allowed » 1))
return 1;
return 0;
}

/* Return the second highest brr task, NULL otherwise */
static struct task_struct *pick_next_highest_task_brr(struct rq *rq, int cpu)
{
struct task_struct *next = NULL;
struct sched_brr_entity *brr_se;
struct brr_prio_array *array;
struct brr_rq *brr_rq;
int index;

for_each_leaf_brr_rq(brr_rq, rq) {
array = &brr_rq-»active;
index = sched_find_first_bit(array-»bitmap);
next_index:
if (index »= MAX_brr_PRIO)
continue;
if (next && next-»prio « index)
continue;
list_for_each_entry(brr_se, array-»queue + index, run_list) {
struct task_struct *p = brr_task_of(brr_se);
if (pick_brr_task(rq, p, cpu)) {
next = p;
break;
}
}
if (!next) {
index = find_next_bit(array-»bitmap, MAX_brr_PRIO, index+1);
goto next_index;
}
}

return next;
}

static DEFINE_PER_CPU(cpumask_var_t, local_cpu_mask);

static inline int pick_optimal_cpu(int this_cpu,
   const struct cpumask *mask)
{
int first;

/* "this_cpu" is cheaper to preempt than a remote processor */
if ((this_cpu != -1) && cpumask_test_cpu(this_cpu, mask))
return this_cpu;

first = cpumask_first(mask);
if (first « nr_cpu_ids)
return first;

return -1;
}

static int find_lowest_rq(struct task_struct *task)
{
struct sched_domain *sd;
struct cpumask *lowest_mask = __get_cpu_var(local_cpu_mask);
int this_cpu = smp_processor_id();
int cpu      = task_cpu(task);
cpumask_var_t domain_mask;

if (task-»brr.nr_cpus_allowed == 1)
return -1; /* No other targets possible */

if (!cpupri_find(&task_rq(task)-»rd-»cpupri, task, lowest_mask))
return -1; /* No targets found */

/*
* Only consider CPUs that are usable for migration.
* I guess we might want to change cpupri_find() to ignore those
* in the first place.
*/
cpumask_and(lowest_mask, lowest_mask, cpu_active_mask);

/*
* At this point we have built a mask of cpus representing the
* lowest priority tasks in the system.  Now we want to elect
* the best one based on our affinity and topology.
*
* We prioritize the last cpu that the task executed on since
* it is most likely cache-hot in that location.
*/
if (cpumask_test_cpu(cpu, lowest_mask))
return cpu;

/*
* Otherwise, we consult the sched_domains span maps to figure
* out which cpu is logically closest to our hot cache data.
*/
if (this_cpu == cpu)
this_cpu = -1; /* Skip this_cpu opt if the same */

if (alloc_cpumask_var(&domain_mask, GFP_ATOMIC)) {
for_each_domain(cpu, sd) {
if (sd-»flags & SD_WAKE_AFFINE) {
int best_cpu;

cpumask_and(domain_mask,
    sched_domain_span(sd),
    lowest_mask);

best_cpu = pick_optimal_cpu(this_cpu,
    domain_mask);

if (best_cpu != -1) {
free_cpumask_var(domain_mask);
return best_cpu;
}
}
}
free_cpumask_var(domain_mask);
}

/*
* And finally, if there were no matches within the domains
* just give the caller *something* to work with from the compatible
* locations.
*/
return pick_optimal_cpu(this_cpu, lowest_mask);
}

/* Will lock the rq it finds */
static struct rq *find_lock_lowest_rq(struct task_struct *task, struct rq *rq)
{
struct rq *lowest_rq = NULL;
int tries;
int cpu;

for (tries = 0; tries « brr_MAX_TRIES; tries++) {
cpu = find_lowest_rq(task);

if ((cpu == -1) || (cpu == rq-»cpu))
break;

lowest_rq = cpu_rq(cpu);

/* if the prio of this runqueue changed, try again */
if (double_lock_balance(rq, lowest_rq)) {
/*
* We had to unlock the run queue. In
* the mean time, task could have
* migrated already or had its affinity changed.
* Also make sure that it wasn't scheduled on its rq.
*/
if (unlikely(task_rq(task) != rq ||
     !cpumask_test_cpu(lowest_rq-»cpu,
       &task-»cpus_allowed) ||
     task_running(rq, task) ||
     !task-»se.on_rq)) {

spin_unlock(&lowest_rq-»lock);
lowest_rq = NULL;
break;
}
}

/* If this rq is still suitable use it. */
if (lowest_rq-»brr.highest_prio.curr » task-»prio)
break;

/* try again */
double_unlock_balance(rq, lowest_rq);
lowest_rq = NULL;
}

return lowest_rq;
}

static inline int has_pushable_tasks(struct rq *rq)
{
return !plist_head_empty(&rq-»brr.pushable_tasks);
}

static struct task_struct *pick_next_pushable_task(struct rq *rq)
{
struct task_struct *p;

if (!has_pushable_tasks(rq))
return NULL;

p = plist_first_entry(&rq-»brr.pushable_tasks,
      struct task_struct, pushable_tasks);

BUG_ON(rq-»cpu != task_cpu(p));
BUG_ON(task_current(rq, p));
BUG_ON(p-»brr.nr_cpus_allowed «= 1);

BUG_ON(!p-»se.on_rq);
BUG_ON(!brr_task(p));

return p;
}

/*
* If the current CPU has more than one brr task, see if the non
* running task can migrate over to a CPU that is running a task
* of lesser priority.
*/
static int push_brr_task(struct rq *rq)
{
struct task_struct *next_task;
struct rq *lowest_rq;

if (!rq-»brr.overloaded)
return 0;

next_task = pick_next_pushable_task(rq);
if (!next_task)
return 0;

retry:
if (unlikely(next_task == rq-»curr)) {
WARN_ON(1);
return 0;
}

/*
* It's possible that the next_task slipped in of
* higher priority than current. If that's the case
* just reschedule current.
*/
if (unlikely(next_task-»prio « rq-»curr-»prio)) {
resched_task(rq-»curr);
return 0;
}

/* We might release rq lock */
get_task_struct(next_task);

/* find_lock_lowest_rq locks the rq if found */
lowest_rq = find_lock_lowest_rq(next_task, rq);
if (!lowest_rq) {
struct task_struct *task;
/*
* find lock_lowest_rq releases rq-»lock
* so it is possible that next_task has migrated.
*
* We need to make sure that the task is still on the same
* run-queue and is also still the next task eligible for
* pushing.
*/
task = pick_next_pushable_task(rq);
if (task_cpu(next_task) == rq-»cpu && task == next_task) {
/*
* If we get here, the task hasnt moved at all, but
* it has failed to push.  We will not try again,
* since the other cpus will pull from us when they
* are ready.
*/
dequeue_pushable_task(rq, next_task);
goto out;
}

if (!task)
/* No more tasks, just exit */
goto out;

/*
* Something has shifted, try again.
*/
put_task_struct(next_task);
next_task = task;
goto retry;
}

deactivate_task(rq, next_task, 0);
set_task_cpu(next_task, lowest_rq-»cpu);
activate_task(lowest_rq, next_task, 0);

resched_task(lowest_rq-»curr);

double_unlock_balance(rq, lowest_rq);

out:
put_task_struct(next_task);

return 1;
}

static void push_brr_tasks(struct rq *rq)
{
/* push_brr_task will return true if it moved an brr */
while (push_brr_task(rq))
;
}

static int pull_brr_task(struct rq *this_rq)
{
int this_cpu = this_rq-»cpu, ret = 0, cpu;
struct task_struct *p;
struct rq *src_rq;

if (likely(!brr_overloaded(this_rq)))
return 0;

for_each_cpu(cpu, this_rq-»rd-»brro_mask) {
if (this_cpu == cpu)
continue;

src_rq = cpu_rq(cpu);

/*
* Don't bother taking the src_rq-»lock if the next highest
* task is known to be lower-priority than our current task.
* This may look racy, but if this value is about to go
* logically higher, the src_rq will push this task away.
* And if its going logically lower, we do not care
*/
if (src_rq-»brr.highest_prio.next »=
    this_rq-»brr.highest_prio.curr)
continue;

/*
* We can potentially drop this_rq's lock in
* double_lock_balance, and another CPU could
* alter this_rq
*/
double_lock_balance(this_rq, src_rq);

/*
* Are there still pullable brr tasks?
*/
if (src_rq-»brr.brr_nr_running «= 1)
goto skip;

p = pick_next_highest_task_brr(src_rq, this_cpu);

/*
* Do we have an brr task that preempts
* the to-be-scheduled task?
*/
if (p && (p-»prio « this_rq-»brr.highest_prio.curr)) {
WARN_ON(p == src_rq-»curr);
WARN_ON(!p-»se.on_rq);

/*
* There's a chance that p is higher in priority
* than what's currently running on its cpu.
* This is just that p is wakeing up and hasn't
* had a chance to schedule. We only pull
* p if it is lower in priority than the
* current task on the run queue
*/
if (p-»prio « src_rq-»curr-»prio)
goto skip;

ret = 1;

deactivate_task(src_rq, p, 0);
set_task_cpu(p, this_cpu);
activate_task(this_rq, p, 0);
/*
* We continue with the search, just in
* case there's an even higher prio task
* in another runqueue. (low likelyhood
* but possible)
*/
}
skip:
double_unlock_balance(this_rq, src_rq);
}

return ret;
}

static void pre_schedule_brr(struct rq *rq, struct task_struct *prev)
{
/* Try to pull brr tasks here if we lower this rq's prio */
if (unlikely(brr_task(prev)) && rq-»brr.highest_prio.curr » prev-»prio)
pull_brr_task(rq);
}

/*
* assumes rq-»lock is held
*/
static int needs_post_schedule_brr(struct rq *rq)
{
return has_pushable_tasks(rq);
}

static void post_schedule_brr(struct rq *rq)
{
/*
* This is only called if needs_post_schedule_brr() indicates that
* we need to push tasks away
*/
spin_lock_irq(&rq-»lock);
push_brr_tasks(rq);
spin_unlock_irq(&rq-»lock);
}

/*
* If we are not running and we are not going to reschedule soon, we should
* try to push tasks away now
*/
static void task_wake_up_brr(struct rq *rq, struct task_struct *p)
{
if (!task_running(rq, p) &&
    !test_tsk_need_resched(rq-»curr) &&
    has_pushable_tasks(rq) &&
    p-»brr.nr_cpus_allowed » 1)
push_brr_tasks(rq);
}

static unsigned long
load_balance_brr(struct rq *this_rq, int this_cpu, struct rq *busiest,
unsigned long max_load_move,
struct sched_domain *sd, enum cpu_idle_type idle,
int *all_pinned, int *this_best_prio)
{
/* don't touch brr tasks */
return 0;
}

static int
move_one_task_brr(struct rq *this_rq, int this_cpu, struct rq *busiest,
struct sched_domain *sd, enum cpu_idle_type idle)
{
/* don't touch brr tasks */
return 0;
}

static void set_cpus_allowed_brr(struct task_struct *p,
const struct cpumask *new_mask)
{
int weight = cpumask_weight(new_mask);

BUG_ON(!brr_task(p));

/*
* Update the migration status of the RQ if we have an brr task
* which is running AND changing its weight value.
*/
if (p-»se.on_rq && (weight != p-»brr.nr_cpus_allowed)) {
struct rq *rq = task_rq(p);

if (!task_current(rq, p)) {
/*
* Make sure we dequeue this task from the pushable list
* before going fubrrher.  It will either remain off of
* the list because we are no longer pushable, or it
* will be requeued.
*/
if (p-»brr.nr_cpus_allowed » 1)
dequeue_pushable_task(rq, p);

/*
* Requeue if our weight is changing and still » 1
*/
if (weight » 1)
enqueue_pushable_task(rq, p);

}

if ((p-»brr.nr_cpus_allowed «= 1) && (weight » 1)) {
rq-»brr.brr_nr_migratory++;
} else if ((p-»brr.nr_cpus_allowed » 1) && (weight «= 1)) {
BUG_ON(!rq-»brr.brr_nr_migratory);
rq-»brr.brr_nr_migratory--;
}

update_brr_migration(&rq-»brr);
}

cpumask_copy(&p-»cpus_allowed, new_mask);
p-»brr.nr_cpus_allowed = weight;
}

/* Assumes rq-»lock is held */
static void rq_online_brr(struct rq *rq)
{
if (rq-»brr.overloaded)
brr_set_overload(rq);

__enable_runtime(rq);

cpupri_set(&rq-»rd-»cpupri, rq-»cpu, rq-»brr.highest_prio.curr);
}

/* Assumes rq-»lock is held */
static void rq_offline_brr(struct rq *rq)
{
if (rq-»brr.overloaded)
brr_clear_overload(rq);

__disable_runtime(rq);

cpupri_set(&rq-»rd-»cpupri, rq-»cpu, CPUPRI_INVALID);
}

/*
* When switch from the brr queue, we bring ourselves to a position
* that we might want to pull brr tasks from other runqueues.
*/
static void switched_from_brr(struct rq *rq, struct task_struct *p,
   int running)
{
/*
* If there are other brr tasks then we will reschedule
* and the scheduling of the other brr tasks will handle
* the balancing. But if we are the last brr task
* we may need to handle the pulling of brr tasks
* now.
*/
if (!rq-»brr.brr_nr_running)
pull_brr_task(rq);
}

static inline void init_sched_brr_class(void)
{
unsigned int i;

for_each_possible_cpu(i)
zalloc_cpumask_var_node(&per_cpu(local_cpu_mask, i),
GFP_KERNEL, cpu_to_node(i));
}
#endif /* CONFIG_SMP */

/*
* When switching a task to brr, we may overload the runqueue
* with brr tasks. In this case we try to push them off to
* other runqueues.
*/
static void switched_to_brr(struct rq *rq, struct task_struct *p,
   int running)
{
int check_resched = 1;

/*
* If we are already running, then there's nothing
* that needs to be done. But if we are not running
* we may need to preempt the current running task.
* If that current running task is also an brr task
* then see if we can move to another run queue.
*/
if (!running) {
#ifdef CONFIG_SMP
if (rq-»brr.overloaded && push_brr_task(rq) &&
    /* Don't resched if we changed runqueues */
    rq != task_rq(p))
check_resched = 0;
#endif /* CONFIG_SMP */
if (check_resched && p-»prio « rq-»curr-»prio)
resched_task(rq-»curr);
}
}

/*
* Priority of the task has changed. This may cause
* us to initiate a push or pull.
*/
static void prio_changed_brr(struct rq *rq, struct task_struct *p,
    int oldprio, int running)
{
if (running) {
#ifdef CONFIG_SMP
/*
* If our priority decreases while running, we
* may need to pull tasks to this runqueue.
*/
if (oldprio « p-»prio)
pull_brr_task(rq);
/*
* If there's a higher priority task waiting to run
* then reschedule. Note, the above pull_brr_task
* can release the rq lock and p could migrate.
* Only reschedule if p is still on the same runqueue.
*/
if (p-»prio » rq-»brr.highest_prio.curr && rq-»curr == p)
resched_task(p);
#else
/* For UP simply resched on drop of prio */
if (oldprio « p-»prio)
resched_task(p);
#endif /* CONFIG_SMP */
} else {
/*
* This task is not running, but if it is
* greater than the current running task
* then reschedule.
*/
if (p-»prio « rq-»curr-»prio)
resched_task(rq-»curr);
}
}

static void watchdog(struct rq *rq, struct task_struct *p)
{
unsigned long soft, hard;

if (!p-»signal)
return;

soft = p-»signal-»rlim[RLIMIT_brrTIME].rlim_cur;
hard = p-»signal-»rlim[RLIMIT_brrTIME].rlim_max;

if (soft != RLIM_INFINITY) {
unsigned long next;

p-»brr.timeout++;
next = DIV_ROUND_UP(min(soft, hard), USEC_PER_SEC/HZ);
if (p-»brr.timeout » next)
p-»cputime_expires.sched_exp = p-»se.sum_exec_runtime;
}
}

static void task_tick_brr(struct rq *rq, struct task_struct *p, int queued)
{
update_curr_brr(rq);

watchdog(rq, p);

/*
* RR tasks need a special form of timeslice management.
* FIFO tasks have no timeslices.
*/
/*If it's not RR, get out now*/
if (p-»policy != SCHED_BRR)
return;

/*If it's not out of time, return*/
if (--p-»brr.time_slice)
return;

p-»brr.time_slice = DEF_TIMESLICE;

/*
* Requeue to the end of queue if we are not the only element
* on the queue:
*/
if (p-»brr.run_list.prev != p-»brr.run_list.next) {
requeue_task_brr(rq, p, 0);
set_tsk_need_resched(p);
}
}

static void set_curr_task_brr(struct rq *rq)
{

//IF 


struct task_struct *p = rq-»curr;

p-»se.exec_stabrr = rq-»clock;

/* The running task is never eligible for pushing */
dequeue_pushable_task(rq, p);
}

static const struct sched_class brr_sched_class = {
.next = &fair_sched_class,
.enqueue_task = enqueue_task_brr,
.dequeue_task = dequeue_task_brr,
.yield_task = yield_task_brr,

.check_preempt_curr = check_preempt_curr_brr,

.pick_next_task = pick_next_task_brr,
.put_prev_task = put_prev_task_brr,

#ifdef CONFIG_SMP
.select_task_rq = select_task_rq_brr,

.load_balance = load_balance_brr,
.move_one_task = move_one_task_brr,
.set_cpus_allowed       = set_cpus_allowed_brr,
.rq_online              = rq_online_brr,
.rq_offline             = rq_offline_brr,
.pre_schedule = pre_schedule_brr,
.needs_post_schedule = needs_post_schedule_brr,
.post_schedule = post_schedule_brr,
.task_wake_up = task_wake_up_brr,
.switched_from = switched_from_brr,
#endif

.set_curr_task          = set_curr_task_brr,
.task_tick = task_tick_brr,

.prio_changed = prio_changed_brr,
.switched_to = switched_to_brr,
};

#ifdef CONFIG_SCHED_DEBUG
extern void print_brr_rq(struct seq_file *m, int cpu, struct brr_rq *brr_rq);

static void print_brr_stats(struct seq_file *m, int cpu)
{
struct brr_rq *brr_rq;

rcu_read_lock();
for_each_leaf_brr_rq(brr_rq, cpu_rq(cpu))
print_brr_rq(m, cpu, brr_rq);
rcu_read_unlock();
}
#endif /* CONFIG_SCHED_DEBUG */