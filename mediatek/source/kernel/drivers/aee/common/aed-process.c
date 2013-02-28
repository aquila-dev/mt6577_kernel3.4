#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/printk.h>
#include <linux/ptrace.h>
#include <linux/ratelimit.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/stacktrace.h>
#include <linux/atomic.h>
#include <linux/module.h>
#include <asm/stacktrace.h>

#include "../aed/aed.h"

struct bt_sync {
	atomic_t cpus_report;
	atomic_t cpus_lock;
};

struct stack_trace_data {
	struct aee_process_bt *trace;
	unsigned int no_sched_functions;
};

static void per_cpu_get_bt(void *info)
{
	struct bt_sync *s = (struct bt_sync *)info;
	atomic_dec(&s->cpus_report);
	while (atomic_read(&s->cpus_lock) == 1);
	atomic_dec(&s->cpus_report);
}

static int save_trace(struct stackframe *frame, void *d)
{
	struct stack_trace_data *data = d;
	struct aee_process_bt *bt = data->trace;
	unsigned long addr = frame->pc;
	
	if (data->no_sched_functions && in_sched_functions(addr)) {
		return 0;
	}

	bt->entries[bt->nr_entries].pc = addr;
	snprintf(bt->entries[bt->nr_entries].symbol, MAX_AEE_KERNEL_SYMBOL, "%pS", (void *)addr);
	bt->nr_entries++;

	return bt->nr_entries >= MAX_AEE_KERNEL_BT;
}

static void aed_get_bt(struct task_struct *tsk, struct aee_process_bt *bt)
{
	struct stack_trace_data data;
	struct stackframe frame;
	int i;

	bt->nr_entries = 0;
	for (i = 0; i < MAX_AEE_KERNEL_BT; i++) {
		bt->entries[i].pc = 0;
		memset(bt->entries[i].symbol, 0, KSYM_SYMBOL_LEN);
	}

	data.trace = bt;

	if (tsk != current) {
		data.no_sched_functions = 1;
		frame.fp = thread_saved_fp(tsk);
		frame.sp = thread_saved_sp(tsk);
		frame.lr = 0;		/* recovered from the stack */
		frame.pc = thread_saved_pc(tsk);
	} else {
		register unsigned long current_sp asm ("sp");

		data.no_sched_functions = 0;
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_sp;
		frame.lr = (unsigned long)__builtin_return_address(0);
		frame.pc = (unsigned long)aed_get_bt;
	}

	walk_stackframe(&frame, save_trace, &data);
	if (bt->nr_entries < MAX_AEE_KERNEL_BT)
		bt->entries[bt->nr_entries++].pc = ULONG_MAX;
}

int aed_get_process_bt(struct aee_process_bt *bt)
{
	int nr_cpus, err;
	struct bt_sync s;
	struct task_struct *task;

	if (bt->pid > 0) {
		task = find_task_by_vpid(bt->pid);
		if (task == NULL) {
			return -EINVAL;
		}
	}
	else {
		return -EINVAL;
	}

	err = mutex_lock_killable(&task->signal->cred_guard_mutex);
        if (err)
                return err;
        if (!ptrace_may_access(task, PTRACE_MODE_ATTACH)) {
                mutex_unlock(&task->signal->cred_guard_mutex);
                return -EPERM;
        }
 
	preempt_disable();

	get_online_cpus();
	nr_cpus = num_online_cpus();
	atomic_set(&s.cpus_report, nr_cpus - 1);
	atomic_set(&s.cpus_lock, 1);

	smp_call_function(per_cpu_get_bt, &s, 0);

	while (atomic_read(&s.cpus_report) != 0);

	aed_get_bt(task, bt);

	atomic_set(&s.cpus_report, nr_cpus - 1);
	atomic_set(&s.cpus_lock, 0);
	while (atomic_read(&s.cpus_report) != 0);

	put_online_cpus();

	preempt_enable();
	
        mutex_unlock(&task->signal->cred_guard_mutex);

	return 0;
}

EXPORT_SYMBOL(aed_get_process_bt);
