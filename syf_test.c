#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "syf-pwm.h"

int fd;

int syf_pmu(cpu_ctrl_t *reg, unsigned long usec)
{
	int i;

	reg->amt_counter = 4;

	reg->evt_t[0] = EVT_INSTRUCTION;
	reg->evt_t[1] = EVT_DTLB_MISS;
	reg->evt_t[2] = EVT_DCACHE_MISS;
	reg->evt_t[3] = EVT_DCACHE_ACCESS;

	printf("INSTRUCTION\tDTLB_MISS\tDCACHE_MISS\tDCACHE_ACCESS\t\tMAR\tDCACHE_MR\tDTLB_MR\n");
	while (1) {
		if (-1 == ioctl(fd, SYFPWM_PMU_START, (void*) reg)) {
			printf("ioctl error.\n");
			return 1;
		}
		usleep((useconds_t) usec);
		if (-1 == ioctl(fd, SYFPWM_PMU_STOP, (void*) reg)) {
			printf("ioctl error.\n");
			return 1;
		}

		for (i = 0; i < reg->amt_counter; ++i) {
			printf("\t%d\t", reg->pmu.counter[i]);
		}
		printf("\t%f\t%f\t%f\t", (float)reg->pmu.counter[2] / reg->pmu.counter[0],
						(float)reg->pmu.counter[2] / reg->pmu.counter[3],
						(float)reg->pmu.counter[1] / reg->pmu.counter[3]);
		printf("\n");

	}
}

int syf_set_freq(cpu_ctrl_t *reg, unsigned long freq)
{
	reg->cpu_freq = freq;

	if (-1 == ioctl(fd, SYFPWM_SET_FEQU, (void*) reg)) {
		printf("ioctl error.\n");
		return 1;
	}

}

unsigned long syf_get_freq(cpu_ctrl_t *reg)
{
	unsigned long freq;

	if (-1 == ioctl(fd, SYFPWM_GET_FEQU, (void*) reg)) {
		printf("ioctl error.\n");
		return 1;
	}

	freq = reg->cpu_freq;
	return freq;
}

int main(int argc, char *argv[])
{
	int i, j;
	cpu_ctrl_t reg;
	unsigned long ctl = atoi(argv[1]);
	unsigned long freq = atoi(argv[2]);
	unsigned long usec = atoi(argv[3]);

	reg.amt_cpu = 1;

	fd = open("/dev/syf-pwm", O_RDWR);
	if (fd == -1) {
		printf("open error.\n");
		return 1;
	}

	switch (ctl) {
	case SYFPWM_APP_PMU:
		syf_pmu(&reg, usec);
		break;

	case SYFPWM_APP_SET_FREQ:
		syf_set_freq(&reg, freq*1000);
		break;

	case SYFPWM_APP_GET_FREQ:
		freq = syf_get_freq(&reg);
		printf("freq = %d\n", freq);
		break;
	}

	return 0;
}
