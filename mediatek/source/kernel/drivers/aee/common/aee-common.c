#include <linux/module.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/kgdb.h>
#include <linux/kdb.h>
#include <linux/utsname.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/init.h>
#include <linux/smp.h>
#include <linux/mt_sched_mon.h>
#include <linux/io.h>
#include <linux/delay.h>

#undef WDT_DEBUG_VERBOSE

#ifdef CONFIG_ARCH_MT6577
#include <mach/mt6577_reg_base.h>
#define GIC_ICDISER0 (GIC_DIST_BASE + 0x100)
#define GIC_ICDISER1 (GIC_DIST_BASE + 0x104)
#define GIC_ICDISER2 (GIC_DIST_BASE + 0x108)
#define GIC_ICDISER3 (GIC_DIST_BASE + 0x10C)
#define GIC_ICDISER4 (GIC_DIST_BASE + 0x110)
#endif

#ifdef CONFIG_HAVE_ARM_TWD
extern int dump_localtimer_register(char* buffer, int size);
#endif

#define THREAD_INFO(sp) ((struct thread_info *) \
				((unsigned long)(sp) & ~(THREAD_SIZE - 1)))

#ifdef CONFIG_SCHED_DEBUG
extern int sysrq_sched_debug_show(void);
#endif

extern void mtk_wdt_disable(void);
#ifdef CONFIG_LOCAL_WDT
enum wk_wdt_type {
	WK_WDT_LOC_TYPE,
	WK_WDT_EXT_TYPE,
	WK_WDT_LOC_TYPE_NOLOCK,
	WK_WDT_EXT_TYPE_NOLOCK,	
};

extern void mpcore_wk_wdt_stop(void);
extern void mpcore_wdt_restart(enum wk_wdt_type type);
extern void mtk_wdt_restart(enum wk_wdt_type type);
#else
extern void mtk_wdt_restart(void);
#endif

#ifdef CONFIG_SMP
//extern void dump_log_idle(void);
extern void irq_raise_softirq(const struct cpumask *mask, unsigned int irq);
#endif
extern void mt_fiq_printf(const char *fmt, ...);

#define KERNEL_REPORT_LENGTH	1024
#define WDT_LOG_DEFAULT_SIZE 	4096

#define AEK_LOG_TAG "aee/aek"

extern int debug_locks;
static struct aee_kernel_api *g_aee_api = NULL;
static char wdt_log_buf[WDT_LOG_DEFAULT_SIZE];
static int wdt_log_length = 0;
static atomic_t wdt_enter_fiq;

void aee_wdt_dump_info(void)
{
	char *printk_buf = wdt_log_buf;
	struct task_struct *task ;
	task = &init_task ;

	if (wdt_log_length == 0) {
		printk(KERN_ERR "\n No log for WDT \n");
		mt_dump_sched_traces();
		return;
	}

	// printk temporary buffer printk_buf[1024]. To avoid char loss, add 4 bytes here
	while (wdt_log_length > 0) {
		printk(KERN_ERR "%s", printk_buf);
		printk_buf += 1020;
		wdt_log_length -= 1020;
	}

	#ifdef CONFIG_SCHED_DEBUG
	sysrq_sched_debug_show();
	#endif

	#ifdef WDT_DEBUG_VERBOSE
	#ifdef CONFIG_SMP
	//dump_log_idle();
	#endif
	#endif

	for_each_process(task)
	{
		if (task->state == 0)
		{
			printk(KERN_ERR "PID: %d, name: %s\n", task->pid, task->comm);
			show_stack(task, NULL);
			printk(KERN_ERR "\n");
		}
		#ifdef WDT_DEBUG_VERBOSE
		if (strncmp(task->comm, "wdtk", 4)==0)
		{
			printk(KERN_ERR "PID: %d, name: %s\n", task->pid, task->comm);
			show_stack(task, NULL);
			printk(KERN_ERR "\n");
		}
		#endif
	}

	#ifdef WDT_DEBUG_VERBOSE
	printk(KERN_ERR "Backtrace of current task:\n");
	show_stack(NULL, NULL);
	#endif
}

void aee_wdt_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	wdt_log_length += vsnprintf((wdt_log_buf+wdt_log_length), 
						(sizeof(wdt_log_buf) - wdt_log_length), fmt, args);
	va_end(args);
}

#ifdef CONFIG_SMP
void aee_smp_send_stop(void)
{
	unsigned long timeout;

	if (num_online_cpus() > 1) {
		cpumask_t mask = cpu_online_map;
		int cpu = 0;
		
		asm volatile("MRC p15,0,%0,c0,c0,5\n"
					 "AND %0,%0,#0xf\n"
					 : "+r" (cpu)
					 :
					 : "cc");
		cpu_clear(cpu, mask);
		irq_raise_softirq(&mask, IPI_CPU_STOP);
	}

	// Wait up to one second for other CPUs to stop 
	timeout = USEC_PER_SEC;
	while (num_online_cpus() > 1 && timeout--)
		udelay(1);

	if (num_online_cpus() > 1)
		aee_wdt_printf("WDT: failed to stop secondary CPUs\n");
}
#endif

void aee_wdt_irq_info(void)
{
	#ifdef CONFIG_LOCAL_WDT
	mtk_wdt_restart(WK_WDT_EXT_TYPE_NOLOCK);
	#else
	mtk_wdt_restart();
	#endif

	#ifdef CONFIG_SMP
	aee_smp_send_stop();
	#endif

	aee_wdt_printf("Qwdt: preempt_count=0x%08lx \n", preempt_count());

	#ifdef CONFIG_ARCH_MT6577
	aee_wdt_printf("GIC_ICDISER0=%08lx, GIC_ICDISER1=%08lx, GIC_ICDISER2=%08lx \n", 
					(*(volatile u32 *)(GIC_ICDISER0)), (*(volatile u32 *)(GIC_ICDISER1)), (*(volatile u32 *)(GIC_ICDISER2)));
	aee_wdt_printf("GIC_ICDISER3=%08lx, GIC_ICDISER4=%08lx \n", 
					(*(volatile u32 *)(GIC_ICDISER3)), (*(volatile u32 *)(GIC_ICDISER4)));
	#endif

	#ifdef CONFIG_HAVE_ARM_TWD
	wdt_log_length += dump_localtimer_register((wdt_log_buf+wdt_log_length), (sizeof(wdt_log_buf) - wdt_log_length));
	#endif

	#ifdef CONFIG_MT_SCHED_MONITOR
	mt_aee_dump_sched_traces();
	#endif

	aee_sram_fiq_log(wdt_log_buf);

	/* avoid lock prove to dump_stack in __debug_locks_off() */
	xchg(&debug_locks, 0);

	BUG();
}

extern int get_memory_size(void);
static void aee_dump_stack_top(unsigned long bottom,
		     unsigned long top)
{
    int count = 0;
    unsigned long p;

    /*should check stack address in kernel range*/
    if (bottom & 3) {
        aee_wdt_printf("%s bottom unaligned %08lx \n", __func__, bottom);
        return;
    }
    if (!((bottom >= (PAGE_OFFSET + THREAD_SIZE)) && 
                (bottom <= (PAGE_OFFSET + get_memory_size())))
       ){
        aee_wdt_printf("%s bottom out of kernel addr space %08lx \n", __func__, bottom);
        return;
    }

    if (!((top >= (PAGE_OFFSET + THREAD_SIZE)) && 
                (top <= (PAGE_OFFSET + get_memory_size())))
       ){
        aee_wdt_printf("%s top out of kernel addr space %08lx \n", __func__, top);
        return;
    }

    aee_wdt_printf("stack (0x%08lx to 0x%08lx)\n",bottom, top);

    for (p = bottom; p < top; p += 4) {
        unsigned long val;
        if(count == 0)
        {
            aee_wdt_printf("%04lx: ", p & 0xffff);
        }
        val = *((unsigned long *)(p));
        aee_wdt_printf("%08lx ",val);
        
        count++;
        if(count == 8)
        {
            aee_wdt_printf("\n");
            count = 0;
        }
    }
}

void aee_wdt_fiq_info(void *arg, void *regs, void *svc_sp)
{
#if defined(CONFIG_FIQ_DEBUGGER)
	register int sp asm("sp");
	struct pt_regs *ptregs = (struct pt_regs *)regs;

	mt_fiq_printf("Triggered :cpu-%d\n", THREAD_INFO(svc_sp)->cpu);

	asm volatile("mov %0, %1\n\t"
				"mov fp, %2\n\t"
				: "=r" (sp)
				: "r" (svc_sp), "r" (ptregs->ARM_fp)
				);

	if (atomic_xchg(&wdt_enter_fiq, 1) != 0) {
		mt_fiq_printf("\n Other CPU already enter WDT FIQ handler \n");
		// loop forever here to avoid SMP deadlock risk during panic flow
		while(1);
	}

	aee_wdt_printf("CPU %d FIQ: Watchdog time out\n", THREAD_INFO(svc_sp)->cpu);

	aee_wdt_printf("pc  : %08lx, lr : %08lx, cpsr : %08lx\n",
				((struct pt_regs *)regs)->ARM_pc,
				((struct pt_regs *)regs)->ARM_lr,
				((struct pt_regs *)regs)->ARM_cpsr);
	aee_wdt_printf("sp  : %08lx, ip : %08lx, fp : %08lx\n",
				((struct pt_regs *)regs)->ARM_sp,
				((struct pt_regs *)regs)->ARM_ip,
				((struct pt_regs *)regs)->ARM_fp);
	aee_wdt_printf("r10 : %08lx, r9 : %08lx, r8 : %08lx\n",
				((struct pt_regs *)regs)->ARM_r10,
				((struct pt_regs *)regs)->ARM_r9,
				((struct pt_regs *)regs)->ARM_r8);
	aee_wdt_printf("r7  : %08lx, r6 : %08lx, r5 : %08lx\n",
				((struct pt_regs *)regs)->ARM_r7,
				((struct pt_regs *)regs)->ARM_r6,
				((struct pt_regs *)regs)->ARM_r5);
	aee_wdt_printf("r4  : %08lx, r3 : %08lx, r2 : %08lx\n",
				((struct pt_regs *)regs)->ARM_r4,
				((struct pt_regs *)regs)->ARM_r3,
				((struct pt_regs *)regs)->ARM_r2);
	aee_wdt_printf("r1  : %08lx, r0 : %08lx\n",
				((struct pt_regs *)regs)->ARM_r1,
				((struct pt_regs *)regs)->ARM_r0);

    aee_dump_stack_top(((struct pt_regs *)regs)->ARM_sp, 
                       ((struct pt_regs *)regs)->ARM_sp + 256);
	aee_wdt_irq_info();
#endif  /* CONFIG_FIQ_DEBUGGER */
}

#ifdef CONFIG_KGDB_KDB
/* Press key to enter kdb */
void aee_trigger_kdb(void)
{
	/* disable Watchdog HW, note it will not enable WDT again when kdb return */
	mtk_wdt_disable();
	#ifdef CONFIG_LOCAL_WDT
	/* disable all local WDT on some specific SMP platform */
	#ifdef CONFIG_SMP
	on_each_cpu((smp_call_func_t)mpcore_wk_wdt_stop, NULL, 0);
	#else
	mpcore_wk_wdt_stop();
	#endif
	#endif
	
	#ifdef CONFIG_SCHED_DEBUG
	sysrq_sched_debug_show();
	#endif
	
	printk(KERN_INFO "User trigger KDB \n");
	mtk_set_kgdboc_var();
	kgdb_breakpoint();
	
	printk(KERN_INFO "Exit KDB \n");
	#ifdef CONFIG_LOCAL_WDT	
	/* enable local WDT */
	#ifdef CONFIG_SMP
	on_each_cpu((smp_call_func_t)mpcore_wdt_restart, WK_WDT_LOC_TYPE, 0);
	#else
	mpcore_wdt_restart(WK_WDT_LOC_TYPE);
	#endif
	#endif
	
}
#else
/* For user mode or the case KDB is not enabled, print basic debug messages */
void aee_dumpbasic(void)
{
	struct task_struct *p = current;
	int orig_log_level = console_loglevel;

	preempt_disable();
	console_loglevel = 7;
	printk(KERN_INFO "kernel  : %s-%s \n", init_uts_ns.name.sysname, init_uts_ns.name.release);
	printk(KERN_INFO "version : %s \n", init_uts_ns.name.version);
	printk(KERN_INFO "machine : %s \n\n", init_uts_ns.name.machine);

	#ifdef CONFIG_SCHED_DEBUG
	sysrq_sched_debug_show();
	#endif
	printk(KERN_INFO "\n%-*s      Pid   Parent Command \n", (int)(2*sizeof(void *))+2, "Task Addr");
	printk(KERN_INFO "0x%p %8d %8d  %s \n\n", (void *)p, p->pid, p->parent->pid, p->comm);
	printk(KERN_INFO "Stack traceback for current pid %d \n", p->pid);
	show_stack(p, NULL);
	console_loglevel = orig_log_level;
	preempt_enable();
}

void aee_trigger_kdb(void)
{
	printk(KERN_INFO "\nKDB is not enabled ! Dump basic debug info... \n\n");
	aee_dumpbasic();
}
#endif

struct aee_oops *aee_oops_create(AE_DEFECT_ATTR attr, AE_EXP_CLASS clazz, const char *module)
{
	struct aee_oops *oops = kzalloc(sizeof(struct aee_oops), GFP_KERNEL | GFP_ATOMIC);
	oops->attr = attr;
	oops->clazz = clazz;
	if (module != NULL) {
		strlcpy(oops->module, module, sizeof(oops->module));
	}
	else {
		strcpy(oops->module, "N/A");
	}
	strcpy(oops->backtrace, "N/A");
	strcpy(oops->process_path, "N/A");
	
	return oops;
}
EXPORT_SYMBOL(aee_oops_create);

void aee_oops_set_process_path(struct aee_oops *oops, const char *process_path) 
{
	if (process_path != NULL) {
		strlcpy(oops->process_path, process_path, sizeof(oops->process_path));
	}
}

void aee_oops_set_backtrace(struct aee_oops *oops, const char *backtrace) 
{
	if (backtrace != NULL) {
		strlcpy(oops->backtrace, backtrace, sizeof(oops->backtrace));
	}
}

void aee_oops_free(struct aee_oops *oops) 
{
	if (oops->detail) {
		kfree(oops->detail);
	}
	if (oops->console) {
		kfree(oops->console);
	}
	if (oops->android_main)	{
		kfree (oops->android_main);
	}
	if (oops->android_radio) {
		kfree (oops->android_radio);
	}
	if (oops->android_system) {
		kfree (oops->android_system);
	}  
	if (oops->userspace_info) {
		kfree (oops->userspace_info);
	}
	kfree(oops);
}

EXPORT_SYMBOL(aee_oops_free);

int aee_register_api(struct aee_kernel_api *aee_api)
{
	if (!aee_api) {
		BUG();
	}

	g_aee_api = aee_api;
	return 0;
}
EXPORT_SYMBOL(aee_register_api);

void aee_kernel_exception_api(const char *file, const int line, const char *module, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	if(g_aee_api && g_aee_api->kernel_exception) {
		char *msgbuf = kmalloc(KERNEL_REPORT_LENGTH, GFP_KERNEL | GFP_ATOMIC);
		char *msgbuf2 = kmalloc(KERNEL_REPORT_LENGTH, GFP_KERNEL | GFP_ATOMIC);
		vsnprintf(msgbuf, KERNEL_REPORT_LENGTH, msg, args);
		snprintf(msgbuf2, KERNEL_REPORT_LENGTH, "<%s:%d> %s", file, line, msgbuf);
		g_aee_api->kernel_exception(module, msgbuf2);
		kfree(msgbuf);
	} else {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s: ", module);
		vprintk(msg, args);
	}
	va_end(args);
}
EXPORT_SYMBOL(aee_kernel_exception_api);

void aee_kernel_warning_api(const char *file, const int line, const char *module, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	if(g_aee_api && g_aee_api->kernel_warning) {
		char *msgbuf = kmalloc(KERNEL_REPORT_LENGTH, GFP_KERNEL | GFP_ATOMIC);
		char *msgbuf2 = kmalloc(KERNEL_REPORT_LENGTH, GFP_KERNEL | GFP_ATOMIC);
		vsnprintf(msgbuf, KERNEL_REPORT_LENGTH, msg, args);
		snprintf(msgbuf2, KERNEL_REPORT_LENGTH, "<%s:%d> %s", file, line, msgbuf);
		g_aee_api->kernel_warning(module, msgbuf2);
		kfree(msgbuf);
	} else {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s: ", module);
		vprintk(msg, args);
	}
	va_end(args);
}
EXPORT_SYMBOL(aee_kernel_warning_api);

void aee_kernel_reminding_api(const char *file, const int line, const char *module, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	if(g_aee_api && g_aee_api->kernel_reminding) {
		char *msgbuf = kmalloc(KERNEL_REPORT_LENGTH, GFP_KERNEL | GFP_ATOMIC);
		char *msgbuf2 = kmalloc(KERNEL_REPORT_LENGTH, GFP_KERNEL | GFP_ATOMIC);
		vsnprintf(msgbuf, KERNEL_REPORT_LENGTH, msg, args);
		snprintf(msgbuf2, KERNEL_REPORT_LENGTH, "<%s:%d> %s", file, line, msgbuf);
		g_aee_api->kernel_reminding(module, msgbuf2);
		kfree(msgbuf);
	} else {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s: ", module);
		vprintk(msg, args);
	}
	va_end(args);
}
EXPORT_SYMBOL(aee_kernel_reminding_api);


void aed_md_exception1(const int *log, int log_size, const int *phy, int phy_size, const char *assert_type, const char *exp_filename, unsigned int exp_linenum, unsigned int fatal1, unsigned int fatal2)
{
	if(g_aee_api && g_aee_api->md_exception1)
		g_aee_api->md_exception1(log, log_size, phy,phy_size,assert_type,
								exp_filename,exp_linenum,fatal1,fatal2);  
}
EXPORT_SYMBOL(aed_md_exception1);

void aed_md_exception2(const int *log, int log_size, const int *phy, int phy_size, const char* detail)
{
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "aed_md_exception2\n") ;
	if(g_aee_api && g_aee_api->md_exception2)
	{
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "aed_md_exception2 will call 0x%x\n", g_aee_api->md_exception2) ; 
		g_aee_api->md_exception2(log, log_size, phy,phy_size,detail);
	} else if(g_aee_api)
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "g_aee_api->md_exception2 = 0x%x\n", g_aee_api->md_exception2) ;
	else 
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "g_aee_api is null\n");

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG,  "aed_md_exception2 out\n");
}
EXPORT_SYMBOL(aed_md_exception2);

char sram_printk_buf[256];

void ram_console_write(struct console *console, const char *s, unsigned int count);

void aee_sram_printk(const char *fmt, ...)
{
	unsigned long long t;
	unsigned long nanosec_rem;
	va_list args;
	int r, tlen;

	va_start(args, fmt);
	
	preempt_disable();
	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	tlen = sprintf(sram_printk_buf, ">%5lu.%06lu< ",
		       (unsigned long) t, nanosec_rem / 1000);

	r = vsnprintf(sram_printk_buf + tlen, sizeof(sram_printk_buf) - tlen, fmt, args);

	ram_console_write(NULL, sram_printk_buf, r + tlen);
	preempt_enable();
	va_end(args);
}
  
EXPORT_SYMBOL(aee_sram_printk);
static int __init aee_common_init(void)
{
    atomic_set(&wdt_enter_fiq, 0);
    memset(wdt_log_buf, 0, sizeof(wdt_log_buf));
    return 0;
}
module_init(aee_common_init);
