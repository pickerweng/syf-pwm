#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mad.h>
#include <fixedmath.h>

#define MAX_NUM_INP		10
#define MAX_NUM_HID		4
#define MAX_NUM_OUT		1

const mad_fixed_t F_POS_ONE  	 = MAD_F_ONE;
const mad_fixed_t F_POS_TWO  	 = mad_f_tofixed((float)2.0);
const mad_fixed_t F_NEG_ONE  	 = mad_f_tofixed((float)-1.0);
const mad_fixed_t F_ZERO	 	 = mad_f_tofixed((float)0.0);
const mad_fixed_t F_POS_RDDO 	 = mad_f_tofixed((float)0.5);
const mad_fixed_t F_NEG_RDDO 	 = mad_f_tofixed((float)-0.5);

#define fixed(v)	mad_f_tofixed(v)

mad_fixed_t W_xh[MAX_NUM_INP][MAX_NUM_HID], W_hy[MAX_NUM_HID][MAX_NUM_OUT];
mad_fixed_t Q_h[MAX_NUM_HID], Q_y[MAX_NUM_OUT];
static int num_inp;
static int num_hid;
static int num_out;

int syf_BPNN_initial(int num_inp, int num_hid, int num_out,
			float inW_xh[MAX_NUM_INP][MAX_NUM_HID],
			float inW_hy[MAX_NUM_HID][MAX_NUM_OUT],
			float inQ_h[MAX_NUM_HID],
			float inQ_y[MAX_NUM_OUT])
{
	int j, h;

	for (h = 0; h < num_hid; h++) {
		for (i = 0; i < num_inp; i++) {
			W_xh[i][h] = mad_f_tofixed(inW_xh[i][h]);
		}
	}

	for (j = 0; j < num_out; j++) {
		for (h = 0; h < num_hid; h++) {
		 	W_hy[h][j] = mad_f_tofixed(inW_hy[h][j]);
		}
	}

	for (h = 0; h < num_hid; h++) {
		Q_h[h] = mad_f_tofixed(inQ_h[h]);
	}

	for (j = 0; j < num_out; j++) {
		Q_y[j] = mad_f_tofixed(inQ_y[j]);
	}
}

int syf_BPNN(float inp[],
			int num_inp, int num_hid, int num_out)
{
	mad_fixed_t X[MAX_NUM_INP], H[MAX_NUM_HID], Y[MAX_NUM_OUT];
	mad_fixed_t sum, mse, exp_result;
	float result;
	float th[5] = {0.0, 0.3, 0.5, 0.7, 1.0};
	int Itest;
	int i, j, h;
	int ret;

	for (Itest = 0; Itest < num_test; Itest++) {
		for (i = 0; i < num_inp; i++) {
			X[i] = fixed(inp[i]);
		}

		for (h = 0; h < num_hid; h++) {
			sum = F_ZERO;
			for (i = 0; i < num_inp; i++) {
				sum = mad_f_add(sum, mad_f_mul(X[i], W_xh[i][h]));
			}
			/* H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h]))); */
			exp_result = _fixed_exp(mad_f_mul(F_NEG_ONE, mad_f_sub(sum, Q_h[h])));
			H[h] = mad_f_div(F_POS_ONE, mad_f_add(F_POS_ONE, exp_result));
		}

		for (j = 0; j < num_out; j++) {
			sum = F_ZERO;
			for (h = 0; h < num_hid; h++) {
				sum = mad_f_add(sum, mad_f_mul(H[h], W_hy[h][j]));
			}
			/* Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j]))); */
			exp_result = _fixed_exp(mad_f_mul(F_NEG_ONE, mad_f_sub(sum, Q_y[j])));
			Y[j] = mad_f_div(F_POS_ONE, mad_f_add(F_POS_ONE, exp_result));
		}
	}

	/* for one result */
	result = mad_f_todouble(Y[0]);
	if (fabs(result - th[L1]) < 0.15)
		ret = L1;
	else if (fabs(result - th[L2]) < 0.1)
		ret = L2;
	else if (fabs(result - th[L3]) < 0.1)
		ret = L3;
	else if (fabs(result - th[L4]) < 0.1)
		ret = L4;
	else if (fabs(result - th[L5]) < 0.15)
		ret = L5;
	else
		ret = L3;

	return ret;
}

