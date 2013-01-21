#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "syf-pwm.h"

#define MAX_NUM_INP		3
#define MAX_NUM_HID		4
#define MAX_NUM_OUT		1

#define WEIGHT_FILE	"pwm.wei"

	int fd;
	float X[MAX_NUM_INP], T[MAX_NUM_OUT], H[MAX_NUM_HID], Y[MAX_NUM_OUT];
	float W_xh[MAX_NUM_INP][MAX_NUM_HID], W_hy[MAX_NUM_HID][MAX_NUM_OUT];
	float Q_h[MAX_NUM_HID], Q_y[MAX_NUM_OUT];
	float sum, mse;

float parser(FILE *fp)
{
	char c;
	char str[50], *p;
	float res;

	do {
		c = fgetc(fp);
	} while (c == '\t' || c == '\r' || c == ' ' || c == '\n');

	p = str;
	*p++ = c;
	do {
		c = fgetc(fp);
		*p++ = c;
	} while (c != '\t' && c != '\r' && c != ' ' && c != '\n');
	p--;
	*p = '\0';

	res = atof(str);

	return res;
}

int syf_BPNN_initial(int num_inp, int num_hid, int num_out)
{
	int h, i, j;

	FILE *fp1;

	fp1 = fopen(WEIGHT_FILE, "r");
	if (fp1 == NULL) {
		puts("File not exist !!");
		return 1;
	}

	fseek(fp1, 0, 0);
	float tmp;
	for (h = 0; h < num_hid; h++) {
		for (i = 0; i < num_inp; i++) {
			W_xh[i][h] = parser(fp1);
		}
	}

	for (j = 0; j < num_out; j++) {
		for (h = 0; h < num_hid; h++) {
		 	W_hy[h][j] = parser(fp1);
		}
	}

	for (h = 0; h < num_hid; h++) {
		Q_h[h] = parser(fp1);
	}

	for (j = 0; j < num_out; j++) {
		Q_y[j] = parser(fp1);
	}

	fclose(fp1);

	return 0;
}

int syf_BPNN(float inp[],
			int num_inp, int num_hid, int num_out)
{
	float result[MAX_NUM_OUT];
	float th[5] = {0.0, 0.3, 0.5, 0.7, 1.0};
	float tmp;
	int Itest;
	int i, j, h;
	static int ret = L1;
	int num_test = 1;

	for (Itest = 0; Itest < num_test; Itest++) {
		for (i = 0; i < num_inp; i++) {
			X[i] = inp[i];
		}

		for (h = 0; h < num_hid; h++) {
			sum = 0.0;
			for (i = 0; i < num_inp; i++)
				sum = sum + X[i] * W_xh[i][h];
			H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h])));
		}

		for (j = 0; j < num_out; j++) {
			sum = 0.0;
			for (h = 0; h < num_hid; h++)
				sum = sum + H[h] * W_hy[h][j];
			Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j])));
		}
	}

	/* for one result */
	for (i = 0; i < MAX_NUM_OUT; ++i) {
		result[i] = Y[i];
		printf("result=%f\t", result[i]);
	}
	//printf("\n");
	if (fabs(result[0] - th[L1]) < 0.15)
		ret = L1;
	else if (fabs(result[0] - th[L2]) < 0.1)
		ret = L1;
	else if (fabs(result[0] - th[L3]) < 0.1)
		ret = L3;
	else if (fabs(result[0] - th[L4]) < 0.1)
		ret = L5;
	else if (fabs(result[0] - th[L5]) < 0.15)
		ret = L5;
	else
		ret = L3;

	return ret;
}

int syf_pmu(cpu_ctrl_t *reg, unsigned long usec)
{
	int i;

	reg->amt_counter = 4;

	reg->evt_t[0] = EVT_INSTRUCTION;
	reg->evt_t[1] = EVT_DTLB_MISS;
	reg->evt_t[2] = EVT_DCACHE_MISS;
	reg->evt_t[3] = EVT_DCACHE_ACCESS;

	//while (1) {
		if (-1 == ioctl(fd, SYFPWM_PMU_START, (void*) reg)) {
			printf("ioctl error.\n");
			return 1;
		}
		usleep((useconds_t) usec);
		if (-1 == ioctl(fd, SYFPWM_PMU_STOP, (void*) reg)) {
			printf("ioctl error.\n");
			return 1;
		}

#ifdef _DEBUG
		for (i = 0; i < reg->amt_counter; ++i) {
			printf("\t%d\t", reg->pmu.counter[i]);
		}
		printf("\n");
#endif
	//}
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
	unsigned long ctl;
	unsigned long freq;
	unsigned long usec = 100000;
	float inp[MAX_NUM_INP], MAR, DTLB_MR, DCache_MR;
	int ret;

	reg.amt_cpu = 1;

	fd = open("/dev/syf-pwm", O_RDWR);
	if (fd == -1) {
		printf("open error.\n");
		return 1;
	}

	syf_BPNN_initial(MAX_NUM_INP, MAX_NUM_HID, MAX_NUM_OUT);

	freq = syf_get_freq(&reg);

	while (1) {
		sleep(1);
		syf_pmu(&reg, usec);

		u32 instructs = reg.pmu.counter[0];
		u32 DTLB_Miss = reg.pmu.counter[1];
		u32 DCache_Miss = reg.pmu.counter[2];
		u32 DCache_Access = reg.pmu.counter[3];

		MAR = (float)DCache_Miss / instructs;
		DTLB_MR = (float)DTLB_Miss / DCache_Miss;
		DCache_MR = (float)DCache_Miss / DCache_Access;
		inp[0] = MAR;
		inp[1] = DCache_MR;
		inp[2] = DTLB_MR;
		ret = syf_BPNN(inp, MAX_NUM_INP, MAX_NUM_HID, MAX_NUM_OUT);
#ifdef _DEBUG
		printf("MAR=%f\tDCache_MR=%f\tDTLB_MR=%f\tret=%d\n", MAR, DCache_MR, DTLB_MR, ret);
#endif

		if (freq_set[ret] != freq) {
			syf_set_freq(&reg, freq_set[ret]);
			freq = freq_set[ret];
		}
	}

	return 0;
}

