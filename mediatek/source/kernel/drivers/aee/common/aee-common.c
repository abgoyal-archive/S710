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

#define THREAD_INFO(sp) ((struct thread_info *) \
                ((unsigned long)(sp) & ~(THREAD_SIZE - 1)))

#ifdef CONFIG_SCHED_DEBUG
extern int sysrq_sched_debug_show(void);
#endif

extern void mtk_wdt_disable(void);
#ifdef CONFIG_LOCAL_WDT
enum wk_wdt_type {
        WK_WDT_LOC_TYPE,
        WK_WDT_EXT_TYPE
};
extern void mpcore_wk_wdt_stop(void);
extern void mpcore_wdt_restart(enum wk_wdt_type type);
extern void mtk_wdt_restart(enum wk_wdt_type type);
#else
extern void mtk_wdt_restart(void);
#endif

extern void mt_fiq_printf(const char *fmt, ...);

#define KERNEL_REPORT_LENGTH 1024

#define AEK_LOG_TAG "aee/aek"

static struct aee_kernel_api *g_aee_api = NULL;

void aee_wdt_irq_info(void)
{
	struct task_struct *task ;
	task = &init_task ;

	printk(KERN_ERR "Qwdt: -- watchdog time out \n");	
	printk(KERN_ERR "preempt_count=%d, irqs_disabled=0x%x \n", preempt_count(), irqs_disabled());

	#ifdef CONFIG_LOCAL_WDT
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
	#else
	mtk_wdt_restart();
	#endif	
	
	#ifdef CONFIG_SMP
	/* stop other cores */
	smp_send_stop();
	#endif	
	
	for_each_process(task)
	{
		if (task->state == 0)
		{
			printk(KERN_ERR "PID: %d, name: %s\n backtrace:\n", task->pid, task->comm);
			show_stack(task, NULL);
			printk(KERN_ERR "\n");
		}
	}

	printk(KERN_ERR "backtrace of current task:\n");
	show_stack(NULL, NULL);
	BUG();
}

void aee_wdt_fiq_info(void *arg, void *regs, void *svc_sp)
{
#if defined(CONFIG_FIQ_DEBUGGER)
	register int sp asm("sp");
	struct pt_regs *ptregs = (struct pt_regs *)regs;

	mt_fiq_printf("Triggered :cpu-%d\n", THREAD_INFO(svc_sp)->cpu);
	mt_fiq_printf("FIQ: watchdog time out \n");

	asm volatile("mov %0, %1\n\t"
                     "mov fp, %2\n\t"
                     : "=r" (sp)
                     : "r" (svc_sp), "r" (ptregs->ARM_fp)
                     );

	printk(KERN_ERR "FIQ: watchdog time out\n");
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
