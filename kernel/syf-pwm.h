#ifndef __SYF_DVFS_H__
#define __SYF_DVFS_H__

/* app control flag */
#define SYFPWM_APP_PMU			1
#define SYFPWM_APP_SET_FREQ		2
#define SYFPWM_APP_GET_FREQ		4

/* ioctl flag */
#define SYFPWM_TESTING		1
#define SYFPWM_PMU_START	2
#define SYFPWM_PMU_STOP		4
#define SYFPWM_GET_FEQU		8
#define SYFPWM_SET_FEQU		16

/* PMU related definitions */
typedef unsigned long u32;

#define MAX_FREQ_SCALE	5
enum {
	L1,
	L2,
	L3,
	L4,
	L5,
} SCALE_FREQ;

u32 freq_set[MAX_FREQ_SCALE] = {104000, 208000, 312000, 416000, 520000};

typedef enum _counter_t {
	PMN0, PMN1, PMN2, PMN3, 		/* Performance Counter Register */
	CCNT, 							/*	PMU Clock Counter (CCNT)	*/
	PMNC,     						/*	PMU Control Register	*/
	INTEN,
	FLAG,
	EVTSEL,
} counter_t;

#define MAX_EVT_TYPES	21
typedef enum EVT_TYPES {
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

typedef struct _cpu_ctrl_t {
	unsigned int amt_counter;		// amount of performance counter
	unsigned int amt_cpu;			// amount of cpu
	unsigned int cpu_freq;
	pmu_results_t pmu;
	evt_types_t evt_t[MAX_COUNTERS];
} cpu_ctrl_t;


#endif	// __SYF_DVFS_H__

