/*  
 *  read.c - The simplest kernel module.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */

int init_module(void)
{
	unsigned long therm_status_address = 0x19c;
	unsigned long temp_target_address = 0x1a2;
	unsigned long long therm_read;
	unsigned long long target_read;
	asm volatile ("rdmsr": "=A"(therm_read): "c"(therm_status_address));
	printk(KERN_INFO "msr read %llu \n", (therm_read));
	//int bits = 22 - 16 + 1; 
	therm_read >>= 16; 
	therm_read &= (1ULL << (7)) - 1;
	asm volatile ("rdmsr": "=A"(target_read): "c"(temp_target_address));
	printk(KERN_INFO "msr read %llu \n", (target_read));
	//bits = 23 - 16 + 1; 
	target_read >>= 16; 
	target_read &= (1ULL << (8)) - 1; 
	printk(KERN_INFO "msr read %llu \n", (target_read - therm_read));
	/* 
	 * A non 0 return means init_module failed; module can't be loaded. 
	 */
	return 0;
}

void cleanup_module(void)
{
	printk(KERN_INFO "Temperature read finalized\n");
}
