/*
 *  syf_pwm.c
 *
 *  Copyright (C)  2009 Tony Weng <neutron_nebular@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/kernel_stat.h>

#include <linux/sysfs.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/percpu.h>

#include <linux/mutex.h>

//#include "syf-pwm.h"
#include "xscale-pmu.h"

/* ioctl flag */
#define SYFPWM_TESTING		1
#define SYFPWM_PMU_START	2
#define SYFPWM_PMU_STOP		4
#define SYFPWM_GET_FEQU		8
#define SYFPWM_SET_FEQU		16


typedef struct _cpu_ctrl_t {
	unsigned int amt_counter;
	unsigned int amt_cpu;
	unsigned int cpu_freq;
	pmu_results_t pmu;
	evt_types_t evt_t[MAX_COUNTERS];
} cpu_ctrl_t;

static DEFINE_MUTEX(_syf_mutex);
struct syf_info_t {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_wall;
	struct cpufreq_policy cur_policy;
	struct cpufreq_frequency_table freq_table;
	int cpu;
	unsigned int enable;
};
static DEFINE_PER_CPU(struct syf_info_t, _syf_info);

struct syf_pwm_dev {
	struct cdev cdevs;
} *syf_pwm_devp;
int major, minor;
int devno;

static dev_t syf_pwm_dev_num;
#define DEVICE_NAME	"syf-pwm"

/*
 * Tony:
 *	This function is used to start or to end the governor.
 *	When you design your governor, you can added the function below here,
 *	and then you have to added your entry point in this function.
 */
static struct cpufreq_driver *syf_pwm_freq;
static int
__syf_pwm_cpufreq_governor(struct cpufreq_policy *policy,
							unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct syf_info_t *__syf_info = &per_cpu(_syf_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if (!cpu_online(cpu))
			return -EINVAL;

		if (__syf_info->enable)
			break;

		/*
		 * Tony:
		 *	Call your governor at here.
		 *	Important: You may have a mutex lock for calling your governor.
		 */
		break;

	case CPUFREQ_GOV_STOP:
		break;

	/* Change the cpu freq. with either highest freq. or lowest freq. */
	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&_syf_mutex);
		if (policy->max <
			__syf_info->cur_policy.cur) {
				__cpufreq_driver_target(&__syf_info->cur_policy,
										policy->max,
										CPUFREQ_RELATION_H);
		}
		else if (policy->min >
				__syf_info->cur_policy.cur) {
					__cpufreq_driver_target(&__syf_info->cur_policy,
											policy->min,
											CPUFREQ_RELATION_L);
		}
		mutex_unlock(&_syf_mutex);
		break;
	}

	return 0;
}

/******* Power Management End ********/


static int
syf_pwm_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int
syf_pwm_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int
syf_pwm_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, void *arg)
{
	int i;
	unsigned int freq;
	cpu_ctrl_t *cc				= (cpu_ctrl_t *) arg;
	pmu_results_t *r 			= &(cc->pmu);
	unsigned int amt_cpu 		= cc->amt_cpu;
	unsigned int amt_counter 	= cc->amt_counter;

	for (i = 0; i < amt_cpu; ++i) {
		struct syf_info_t *_sinfo = &per_cpu(_syf_info, i);

		switch (cmd) {
		case SYFPWM_TESTING:
			printk("TESTING.\n");
			break;

		case SYFPWM_PMU_START:
			pmu_start(amt_counter, cc->evt_t);
			break;

		case SYFPWM_PMU_STOP:
			pmu_stop(amt_counter);
			memcpy(r, &pmu, sizeof(pmu_results_t));
			break;

		case SYFPWM_GET_FEQU:
			mutex_lock(&_syf_mutex);
			cpufreq_get_policy(&_sinfo->cur_policy, i);
			freq = cpufreq_get(_sinfo->cur_policy.cpu);
			mutex_unlock(&_syf_mutex);
			break;

		case SYFPWM_SET_FEQU:
			mutex_lock(&_syf_mutex);
			cpufreq_get_policy(&_sinfo->cur_policy, i);
			freq = __cpufreq_driver_target(&_sinfo->cur_policy,
											(unsigned int) cc->cpu_freq,
											CPUFREQ_RELATION_H);
			mutex_unlock(&_syf_mutex);
			break;
		}
	}

	return 0;
}

/* For governor */
static struct cpufreq_governor __syf_cpufreq_gov = {
	.name		= "syf-pwm",
	.governor	= __syf_pwm_cpufreq_governor,
	.owner		= THIS_MODULE,
};

static struct file_operations syf_pwm_fops = {
	.owner		=	THIS_MODULE,
	.open		=	syf_pwm_open,
	.release	=	syf_pwm_release,
	.ioctl		=	syf_pwm_ioctl,
};

static int __init
syf_pwm_init(void)
{
	if (alloc_chrdev_region(&syf_pwm_dev_num, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Can't register device.\n");
		return -1;
	}
	major = MAJOR(syf_pwm_dev_num);
	minor = MINOR(syf_pwm_dev_num);

	syf_pwm_devp = kmalloc(sizeof(struct syf_pwm_dev), GFP_ATOMIC);

	/* Initialization of cdev */
	devno = MKDEV(major, minor);
	cdev_init(&syf_pwm_devp->cdevs, &syf_pwm_fops);
	syf_pwm_devp->cdevs.owner = THIS_MODULE;
	syf_pwm_devp->cdevs.ops = &syf_pwm_fops;
	if (cdev_add(&syf_pwm_devp->cdevs, devno, 1)) {
		printk("Bad cdev.\n");
		return 1;
	}

	/* Initialization of Syf CPUFreq governor */
	cpufreq_register_governor(&__syf_cpufreq_gov);

	/* Initialization of PMU */
	pmu_claim();

	printk("Syf-PWM Driver Registered.\n");
	return 0;
}

static void __exit
syf_pwm_cleanup(void)
{
	cdev_del(&syf_pwm_devp->cdevs);
	unregister_chrdev_region(syf_pwm_dev_num, 1);

	kfree(syf_pwm_devp);

	/* Cleanup of Syf CPUFreq governor */
	cpufreq_unregister_governor(&__syf_cpufreq_gov);

	/* Release PMU */
	pmu_release(id);

	printk("Syf-PWM Driver Unregistered.\n");

	return;
}

module_init(syf_pwm_init);
module_exit(syf_pwm_cleanup);

MODULE_AUTHOR("Tony Weng");
MODULE_LICENSE("GPL");
