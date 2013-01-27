/*
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *  Modified by Caesar 2007/12/17
 *
 *  Modified	2007/12/17	Caesar Chen
 *	Modified	2009		Picker Weng
 */

/*
 *	Change Log:
 *		2009/08/16 By Picker Weng:
 *		- Added dynamical counter selection. You can determine how many counters
 *			you want to use. (How many events you want to catch?) - in XScale PXA270,
 *			it provides four performance counters for user.
 *		- Fixed the interrupt flag problem in Linux Kernel 2.6.25. (SA_INTERRUPT)
 *		- Added some symbols for programmer to recognize which the counter is used.
 */

/*
 *	Picker Weng :
 *	Usage:
 *		You have to follow these steps to catch values
 *		from performance counters.
 *
 *	pmu_claim -> pmu_start -> pmu_stop -> pmu_release
 */

#ifndef _PMU_H_
#define _PMU_H_

#include <linux/types.h>
#include <linux/sysfs.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/version.h>

#include <asm/atomic.h>
#include <asm/arch/irqs.h>

/*
 * Different types of events that can be counted by the XScale PMU
 */
typedef enum {
	PMN0, PMN1, PMN2, PMN3, 		/* Performance Counter Register */
	CCNT, 							/*	PMU Clock Counter (CCNT)	*/
	PMNC,     						/*	PMU Control Register	*/
	INTEN,
	FLAG,
	EVTSEL,
} counter_t;

#define	PMU_ENABLE	0x001					/* Enable counters */
#define PMN_RESET	0x002					/* Reset event counters */
#define	CCNT_RESET	0x004					/* Reset clock counter */
#define	PMU_RESET	CCNT_RESET | PMN_RESET
#define	CLK_DIV		0x008					/* Clock divide enbable */

#define	PMN3_OVERFLOW	0x10	/* Perfromance counter 0 overflow */
#define PMN2_OVERFLOW	0x08	/* Performance counter 1 overflow */
#define PMN1_OVERFLOW	0x04	/* Performance counter 1 overflow */
#define PMN0_OVERFLOW	0x02	/* Performance counter 1 overflow */
#define CCNT_OVERFLOW	0x01	/* Clock counter overflow */

static atomic_t usage = ATOMIC_INIT(0);
static unsigned long id = 0;
static u32 pmnc;
static u32 evtsel;

/*
 * Different types of events that can be counted by the XScale PMU
 */
#define MAX_EVT_TYPES	21
u32 EVT_VALUE[MAX_EVT_TYPES] = {
	0x00,	// EVT_ICACHE_MISS
	0x01,	// EVT_ICACHE_NO_DELIVER
	0x02,	// EVT_DATA_STALL
	0x03,	// EVT_ITLB_MISS
	0x04,	// EVT_DTLB_MISS
	0x05,	// EVT_BRANCH
	0x06,	// EVT_BRANCH_MISS
	0x07,	// EVT_INSTRUCTION
	0x08,	// EVT_DCACHE_FULL_STALL
	0x09,	// EVT_DCACHE_FULL_STALL_CONTIG
	0x0A,	// EVT_DCACHE_ACCESS
	0x0B,	// EVT_DCACHE_MISS
	0x0C,	// EVT_DCACE_WRITE_BACK
	0x0D,	// EVT_PC_CHANGED
	0x10,	// EVT_BCU_REQUEST
	0x11,	// EVT_BCU_FULL
	0x12,	// EVT_BCU_DRAIN
	0x14,	// EVT_BCU_ECC_NO_ELOG
	0x15,	// EVT_BCU_1_BIT_ERR
	0x16,	// EVT_RMW
};

typedef enum {
	EVT_ICACHE_MISS,
	EVT_ICACHE_NO_DELIVER,
	EVT_DATA_STALL,
	EVT_ITLB_MISS,
	EVT_DTLB_MISS,
	EVT_BRANCH,
	EVT_BRANCH_MISS,
	EVT_INSTRUCTION,
	EVT_DCACHE_FULL_STALL,
	EVT_DCACHE_FULL_STALL_CONTIG,
	EVT_DCACHE_ACCESS,
	EVT_DCACHE_MISS,
	EVT_DCACE_WRITE_BACK,
	EVT_PC_CHANGED,
	EVT_BCU_REQUEST,
	EVT_BCU_FULL,
	EVT_BCU_DRAIN,
	EVT_BCU_ECC_NO_ELOG,
	EVT_BCU_1_BIT_ERR,
	EVT_RMW,
} evt_types_t;

#define MAX_COUNTERS		5
#define MAX_COUNTER_TYPES	9
typedef struct __pmu_results_t
{
	u32 counter[MAX_COUNTERS];
	u32 ovfcounter[MAX_COUNTER_TYPES];
} pmu_results_t;
pmu_results_t pmu;

/*
 * Functions prototype
 */
#if 0
static irqreturn_t pmu_irq_handler(int, void *, struct pt_regs *);
void pmu_reg_write(counter_t ct, u32 _value);
u32 pmu_reg_read(counter_t ct);

int pmu_claim(void);						/* Claim PMU for usage */
int pmu_start(unsigned int num_counter,
			evt_types_t *evt_t);	/* Start PMU execution */
int pmu_stop(unsigned int num_counter);	/* Stop perfmon unit */
int pmu_release(int);						/* Release PMU */
#endif

/*
 * Functions implement
 */
/*
*
* pmu_reg_write - Writes to the PMU Register
*
* Description:
* 	This routine writes to the designated PMU register via CoProcesser 14.
*
* Input Parameters:
*       regno - PMU register number to write
*       value - Value to write to PMU register
*
* 	Author: 	Caesar Chen 2007/11/14
*	Modified: 	Picker Weng	2009
*/

inline void pmu_reg_write(counter_t count_type, u32 _value)
{
	switch (count_type) {
		case PMNC:
			asm("mcr p14,0,%0,c0,c1,0" : : "r" (_value));
			break;

		case CCNT:
			asm("mcr p14,0,%0,c1,c1,0" : : "r" (_value));
			break;

		case PMN0:
			asm("mcr p14,0,%0,c0,c2,0" : : "r" (_value));
			break;

		case PMN1:
			asm("mcr p14,0,%0,c1,c2,0" : : "r" (_value));
			break;

		case PMN2:
			asm("mcr p14,0,%0,c2,c2,0" : : "r" (_value));
			break;

		case PMN3:
			asm("mcr p14,0,%0,c3,c2,0" : : "r" (_value));
			break;

		case INTEN:
			asm("mcr p14,0,%0,c4,c1,0" : : "r" (_value));
			break;

		case FLAG:
			asm("mcr p14,0,%0,c5,c1,0" : : "r" (_value));
			break;

		case EVTSEL:
			asm("mcr p14,0,%0,c8,c1,0" : : "r" (_value));
			break;
	}
}

/*
*
* pmu_reg_read - Read the PMU Register
*
* Description:
* 	This routine reads the designated PMU register via CoProcesser 14.
*
* Input Parameters:
*       regno - PMU register number to read.  Number between 0 to 8
*
*		0 -> PMNC,  PMU Control Register
*		1 -> CCNT,  PMU Clock Counter
*		2 -> PMN0,  PMU Count Register 0
*		3 -> PMN1,  PMU Count Register 1
*		4 -> PMN2,	PMU Count Register 2
*		5 -> PMN3,  PMU Count Register 3
*		6 -> INTEN, PMU Interupt Enable Register
*		7 -> FLAG,  PMU Overflow Flag Status Register
*		8 -> EVTSEL	PMU Event Select Register
*
* 	Author: 	Caesar Chen 2007/11/14
*	Modified:	Picker Weng	2009
*/
inline u32 pmu_reg_read(counter_t count_type)
{
	u32 _value = 0;
	switch (count_type) {
		case PMNC:
			asm volatile ("mrc p14,0,%0,c0,c1,0" : "=r" (_value));
			break;

		case CCNT:
			asm volatile ("mrc p14,0,%0,c1,c1,0" : "=r" (_value));
			break;

		case PMN0:
			asm volatile ("mrc p14,0,%0,c0,c2,0" : "=r" (_value));
			break;

		case PMN1:
			asm volatile ("mrc p14,0,%0,c1,c2,0" : "=r" (_value));
			break;

		case PMN2:
			asm volatile ("mrc p14,0,%0,c2,c2,0" : "=r" (_value));
			break;

		case PMN3:
			asm volatile ("mrc p14,0,%0,c3,c2,0" : "=r" (_value));
			break;

		case INTEN:
			asm volatile ("mrc p14,0,%0,c4,c1,0" : "=r" (_value));
			break;

		case FLAG:
			asm volatile ("mrc p14,0,%0,c5,c1,0" : "=r" (_value));
			break;

		case EVTSEL:
			asm volatile ("mrc p14,0,%0,c8,c1,0" : "=r" (_value));
			break;
	}

	return _value;
}

inline static irqreturn_t pmu_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	pmu_results_t *r = (pmu_results_t *) dev_id;
	unsigned int flag;

	/* read the status */
	flag = pmu_reg_read(FLAG);
	pmnc = pmu_reg_read(PMNC);

	if(pmnc & PMN0_OVERFLOW) {
		++r->ovfcounter[PMN0];
	}

	if(pmnc & PMN1_OVERFLOW) {
		++r->ovfcounter[PMN1];
	}

	if(pmnc & PMN2_OVERFLOW) {
		++r->ovfcounter[PMN2];
	}

	if(pmnc & PMN3_OVERFLOW) {
		++r->ovfcounter[PMN3];;
	}

	if(pmnc & CCNT_OVERFLOW) {
		++r->ovfcounter[CCNT];
	}

	pmu_reg_write(FLAG, flag);
	pmu_reg_write(PMNC, pmnc);

	return IRQ_HANDLED;
}

inline int pmu_start(unsigned int num_counter, evt_types_t *evt_t)
{
	int i = -1;
	pmu_results_t *r = &pmu;
	memset(r, 0, sizeof(pmu_results_t));

	for (i = 0; i < num_counter; ++i) {
		switch ((counter_t) i) {
		case PMN0:
		case PMN1:
		case PMN2:
		case PMN3:
			evtsel |= (EVT_VALUE[evt_t[i]] << (i << 3));
			break;

		default:
			break;
		}
	}

	pmnc |= PMU_ENABLE | PMU_RESET;

	pmu_reg_write(EVTSEL, evtsel);
	/*	All interrupt are turned on. */
	pmu_reg_write(INTEN, 0x1F);
	pmu_reg_write(PMNC, pmnc);

	return 0;
}

inline int pmu_stop(unsigned int num_counter)
{
	int i;
	pmu_results_t *r = &pmu;

	if(!pmnc)
		return -ENOSYS;

	r->counter[CCNT] = pmu_reg_read(CCNT);

	for (i = 0; i < num_counter; ++i) {
		switch ((counter_t) i) {
		case PMN0:
		case PMN1:
		case PMN2:
		case PMN3:
			r->counter[i] = pmu_reg_read(i);
			break;

		default:
			break;
		}
	}

	pmnc = 0x0;
	pmu_reg_write(PMNC, pmnc);

	return 0;
}

inline int pmu_claim(void)
{
	int err = 0;

	if(atomic_read(&usage)) {
		return -EBUSY;
		return err;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 18)
	err = request_irq(IRQ_PMU, (irq_handler_t)pmu_irq_handler, IRQF_DISABLED,
			  NULL, (void *) &pmu);
#else
	err = request_irq(IRQ_PMU, (irq_handler_t)pmu_irq_handler, SA_INTERRUPT,
			  NULL, (void *) &pmu);
#endif

	if(err < 0)
	{
		printk(KERN_ERR "unable to request IRQ %d for PXA270 PMU: %d\n",
		       IRQ_PMU, err);
		return err;
	}

	atomic_inc(&usage);
	pmnc = 0x0;
	pmu_reg_write(PMNC, pmnc);
	return ++id;
}

inline int pmu_release(int claim_id)
{
	if(!atomic_read(&usage))
		return 0;

	if(claim_id != id)
		return -EPERM;

	free_irq(IRQ_PMU, (void *) &pmu);
	atomic_dec(&usage);

	return 0;
}

EXPORT_SYMBOL(pmu_claim);
EXPORT_SYMBOL(pmu_release);
EXPORT_SYMBOL(pmu_start);
EXPORT_SYMBOL(pmu_stop);

#endif /*	_PMU_H_	*/


