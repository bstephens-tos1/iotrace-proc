#include "gcc_warnings.h"
PUSH_IGNORE_GCC_WARNINGS();
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include "io_proc.h"

POP_GCC_WARNINGS();
const int  MAX_LEN = 4096;

static struct proc_dir_entry *proc_entry;

// Needs to be big enough to hold at least 2 integers
static char msg[9];

// The path to write out to from within the kernel
const char* log_file = "/mnt/tmpfs/io_trace.log";

/* Our switch to other kernel files to know when to print debug info */
int io_tracing_on = 0;
int ramdisk_fd = 0;
// Use this for checking bounds of buffer position before writing

int io_trace_buf_pos = 0;
char io_trace_buf[IO_TRACE_BUF_SIZE];

EXPORT_SYMBOL(io_tracing_on);
EXPORT_SYMBOL(ramdisk_fd);
EXPORT_SYMBOL(io_trace_buf_pos);
EXPORT_SYMBOL(io_trace_buf);

// Used so we can open a file in the ramdisk
mm_segment_t old_fs;

// ramdisk_fd will be opened after the first io_proc_write
int file_open = 0;

static ssize_t io_proc_write(__maybe_unused struct file* filep, const char __user *buff, size_t len, __maybe_unused loff_t *offset)
{
	
	// Needs to be big enough to hold at least 2 integers
	char info[9] = {0};
	
	if(len > MAX_LEN) {
		printk(KERN_INFO "Exceeded MAX_LEN in write_info()\n");
		return -1;
	}

	if(copy_from_user(&info, buff, len)) {
		printk(KERN_INFO "Error copying from user in write_info()\n");
		return -2;
	}

	if(strcmp(info, "1") == 0) {
		if(!file_open) {
			old_fs = get_fs();
			set_fs(KERNEL_DS);

			ramdisk_fd = sys_open(log_file, O_WRONLY, 0);
			if(ramdisk_fd <= -1) {
				printk(KERN_INFO "IO_TRACE: error opening the ramdisk file; error code is: %d", ramdisk_fd);
				return -1;
			}
			file_open = 1;
		}

		io_trace_buf_pos = 0;
		io_trace_buf[0] = 0;
		io_tracing_on = 1;
	}
	else {
		io_tracing_on = 0;
		sys_write(ramdisk_fd,io_trace_buf,strlen(io_trace_buf)+1);
		sys_close(ramdisk_fd);
		set_fs(old_fs);
		file_open = 0;
		// Userspace can call fsync() if they want the buffer flushed before we reinitialize it
		memset(io_trace_buf, 0, IO_TRACE_BUF_SIZE);
	}
	printk(KERN_INFO "io_switch: %d\n", io_tracing_on);

	return (ssize_t)len;
}

static ssize_t io_proc_read(__maybe_unused struct file *file, char __user *buffer, __maybe_unused size_t len, __maybe_unused loff_t *offset)
{
	static int eof = 0;

	long unsigned int ret = 0;
	ret = sprintf(msg, "%d\t%d\n", io_tracing_on, ramdisk_fd);

	if(eof) {
		//printk(KERN_INFOR "IO_TRACE: read() finished\n");
		eof = 0;
		return 0;
	}
	eof = 1;
	if(copy_to_user(buffer, msg, ret)) {
		printk(KERN_INFO "IO_TRACE: error copying to user in %s\n", __FUNCTION__);
		return -2;
	}
	//printk(KERN_INFO "IO_TRACE: msg: %s buffer: %s io_tracing_on: %d\n", msg, buffer, io_tracing_on);	
	return ret;
}

static const struct file_operations proc_file_fops = {
	.owner		= THIS_MODULE,
	.write		= io_proc_write,
	.read		= io_proc_read,
};

static int __init
io_proc_init(void)
{
	int i, ret = 0;
	io_tracing_on = 0;
	ramdisk_fd = 0;

	for(i = 0; i < sizeof(msg); ++i)
		msg[i] = 0;
 
	proc_entry = proc_create("iotrace", 0644, NULL, &proc_file_fops);

	if(proc_entry == NULL) {
		ret = -1;
		printk(KERN_INFO "iotrace could not be created\n");
	}

	printk(KERN_INFO "iotrace loaded\n");
	return ret;
}

static void __exit
io_proc_exit(void)
{
	remove_proc_entry("iotrace", NULL);
	printk(KERN_INFO "iotrace unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(io_proc_init);
module_exit(io_proc_exit);