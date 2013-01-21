#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mad.h>

#define num_test	1
#define num_inp		2
#define num_hid		4
#define num_out		1
#define weight_file "pwm.wei"
#define test_file   "normal_test.txt"
#define recall_file "xor.rec"

const mad_fixed_t F_POS_ONE  	 = MAD_F_ONE;
const mad_fixed_t F_POS_TWO  	 = mad_f_tofixed((float)2.0);
const mad_fixed_t F_NEG_ONE  	 = mad_f_tofixed((float)-1.0);
const mad_fixed_t F_ZERO	 	 = mad_f_tofixed((float)0.0);
const mad_fixed_t F_POS_RDDO 	 = mad_f_tofixed((float)0.5);
const mad_fixed_t F_NEG_RDDO 	 = mad_f_tofixed((float)-0.5);

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

int main(int argc, char* argv[])
{
	FILE *fp1,*fp2,*fp3;
	mad_fixed_t X[num_inp], T[num_out], H[num_hid], Y[num_out];
	mad_fixed_t W_xh[num_inp][num_hid], W_hy[num_hid][num_out];
	mad_fixed_t Q_h[num_hid], Q_y[num_out];
	mad_fixed_t sum, mse;
	int Itest;
	int i, j, h;

	fp1 = fopen(weight_file, "r");
	fp2 = fopen(test_file, "r");
	fp3 = fopen(recall_file, "w");
	if (fp1 == NULL) {
		puts("File not exist !!");
		getchar();
		exit(1);
	}
	if (fp2 == NULL) {
		puts("File not exist !!");
		getchar();
		exit(1);
	}

	fseek(fp1, 0, 0);
	float tmp;
	for (h = 0; h < num_hid; h++) {
		for (i = 0; i < num_inp; i++) {
			tmp = parser(fp1);
			W_xh[i][h] = mad_f_tofixed(tmp);
		}
	}

	for (j = 0; j < num_out; j++) {
		for (h = 0; h < num_hid; h++) {
		 	tmp = parser(fp1);
		 	W_hy[h][j] = mad_f_tofixed(tmp);
		}
	}

	for (h = 0; h < num_hid; h++) {
		tmp = parser(fp1);
		Q_h[h] = mad_f_tofixed(tmp);
	}

	for (j = 0; j < num_out; j++) {
		tmp = parser(fp1);
		Q_y[j] = mad_f_tofixed(tmp);
	}

	fseek(fp2, 0, 0);
	for (Itest = 0; Itest < num_test; Itest++) {
		for (i = 0; i < num_inp; i++) {
		 	tmp = parser(fp2);
			X[i] = mad_f_tofixed(tmp);
			printf("X[%d]=%f\t", i, tmp);
		}
		printf("\n");

		for (j = 0; j < num_out; j++) {
		 	tmp = parser(fp2);
			T[j] = mad_f_tofixed(tmp);
		}

		for (h = 0; h < num_hid; h++) {
			sum = F_ZERO;
			for (i = 0; i < num_inp; i++) {
				sum = mad_f_add(sum, mad_f_mul(X[i], W_xh[i][h]));
			}
			/* H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h]))); */
			tmp = exp((float)-1.0 * mad_f_todouble(mad_f_sub(sum, Q_h[h])));
			mad_fixed_t exp_result = mad_f_tofixed(tmp);
			H[h] = mad_f_div(F_POS_ONE, mad_f_add(F_POS_ONE, exp_result));
			//printf("H[h]=%f\n", mad_f_todouble(H[h]));
		}

		for (j = 0; j < num_out; j++) {
			sum = F_ZERO;
			for (h = 0; h < num_hid; h++) {
				sum = mad_f_add(sum, mad_f_mul(H[h], W_hy[h][j]));
			}
			/* Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j]))); */
			tmp = exp((float)-1.0 * mad_f_todouble(mad_f_sub(sum, Q_y[j])));
			mad_fixed_t exp_result = mad_f_tofixed(tmp);
			Y[j] = mad_f_div(F_POS_ONE, mad_f_add(F_POS_ONE, exp_result));
		}

		mse = F_ZERO;
		for (j = 0; j < num_out; j++) {
		 	/* mse += (T[j] - Y[j]) * (T[j] - Y[j]); */
			mad_fixed_t first, second;
			first = mad_f_sub(T[j], Y[j]);
			second = mad_f_sub(T[j], Y[j]);
			mse = mad_f_add(mse, mad_f_mul(first, second));
		}

		printf("T[j]= ");
		fprintf(fp3, "T[j]= ");
		for (j = 0; j < num_out; j++) {
			printf("%-8.4f", mad_f_todouble(T[j]));
			fprintf(fp3, "%-8.4f", mad_f_todouble(T[j]));
		}
		printf("Y[j]= ");
		fprintf(fp3, "Y[j]= ");
		for (j = 0; j < num_out; j++) {
			printf("%-8.4f", mad_f_todouble(Y[j]));
			fprintf(fp3,"%-8.4f", mad_f_todouble(Y[j]));
		}
		printf("  mse= %-8.6f\n\n", mad_f_todouble(mse));
		fprintf(fp3, "  mse= %-8.6f", mad_f_todouble(mse));
		fprintf(fp3, "\n");
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	return 0;
}

