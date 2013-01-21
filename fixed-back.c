#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <mad.h>

#define		num_cycle		3000
#define		num_train  		1500
#define		num_inp			2
#define		num_hid    		4
#define		num_out    		1
#define		eta     		0.5
#define		alpha   		0.2
#define		train_file		"test.txt"
#define		weight_file		"pwm.wei"
#define		mse_file		"pwm.mes"

const mad_fixed_t F_POS_ONE  	 = mad_f_tofixed((float)1.0);
const mad_fixed_t F_POS_TWO  	 = mad_f_tofixed((float)2.0);
const mad_fixed_t F_NEG_ONE  	 = mad_f_tofixed((float)-1.0);
const mad_fixed_t F_ZERO	 	 = mad_f_tofixed((float)0.0);
const mad_fixed_t F_POS_RDDO 	 = mad_f_tofixed((float)0.5);
const mad_fixed_t F_NEG_RDDO 	 = mad_f_tofixed((float)-0.5);
const mad_fixed_t F_ALPHA	 	 = mad_f_tofixed((float)alpha);
const mad_fixed_t F_ETA		 	 = mad_f_tofixed((float)eta);
const mad_fixed_t F_NEG_ETA	 	 = mad_f_tofixed((float)-eta);

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

mad_fixed_t random_value(void)
{
	mad_fixed_t result;
	float tmp = (rand()/(RAND_MAX+1.0))-0.5;
	result = mad_f_tofixed(tmp);

	return result;
}

int main(int argc, char* argv[])
{
	FILE *fp1;
	FILE *fp2;
	FILE *fp3;
	mad_fixed_t X[num_inp], T[num_out], H[num_hid], Y[num_out];
	mad_fixed_t W_xh[num_inp][num_hid], W_hy[num_hid][num_out];
	mad_fixed_t dW_xh[num_inp][num_hid], dW_hy[num_hid][num_out];
	mad_fixed_t Q_h[num_hid], Q_y[num_out];
	mad_fixed_t dQ_h[num_hid], dQ_y[num_out];
	mad_fixed_t delta_h[num_hid], delta_y[num_out];
	mad_fixed_t sum, mse;
	int Icycle, Itrain;
	int i, j, h;

	fp1 = fopen(train_file, "r");
	fp2 = fopen(weight_file, "w");
	fp3 = fopen(mse_file, "w");
	if (fp1 == NULL) {
		puts("File not exist !!");
		getchar();
		exit(1);
	}

	srand((int)time(0));

	for (h = 0; h < num_hid; h++) {
		for (i = 0; i < num_inp; i++) {
			W_xh[i][h] = random_value();
			dW_xh[i][h] = F_ZERO;
		}
	}

	for (j = 0; j <= num_out; j++) {
		for (h = 0; h < num_hid; h++) {
			W_hy[h][j] = random_value();
			dW_hy[h][j] = F_ZERO;
		}
	}

	for (h = 0; h < num_hid; h++) {
		Q_h[h] = F_ZERO;
		dQ_h[h] = F_ZERO;
		delta_h[h] = F_ZERO;
	}

	for (j = 0; j < num_out; j++) {
		Q_y[j] = random_value();
		dQ_y[j] = F_ZERO;
		delta_y[j] = F_ZERO;
	}

	/*start learning */
	float tmp;
	for (Icycle = 0; Icycle < num_cycle; Icycle++) {
		mse = F_ZERO;

		fseek(fp1, 0, 0);
		for (Itrain = 0; Itrain < num_train; Itrain++) {
			for (i = 0; i < num_inp; i++) {
				tmp = parser(fp1);
				X[i] = mad_f_tofixed(tmp);
			}


			for (j = 0; j < num_out; j++) {
				tmp = parser(fp1);
				T[j]= mad_f_tofixed(tmp);
			}

			for (h = 0; h < num_hid; h++) {
				sum = F_ZERO;
				for (i = 0; i < num_inp; i++) {
					sum = mad_f_add(sum, mad_f_mul(X[i], W_xh[i][h]));
				}
				/* H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h]))); */
				tmp = exp(-1.0 * mad_f_todouble(mad_f_sub(sum, Q_h[h])));
				mad_fixed_t exp_result = mad_f_tofixed(tmp);
				H[h] = mad_f_div(F_POS_ONE, mad_f_add(F_POS_ONE, exp_result));
			}

			for (j = 0; j < num_out; j++) {
				sum = F_ZERO;
				for (h = 0; h < num_hid; h++) {
					sum = mad_f_add(sum, mad_f_mul(H[h], W_hy[h][j]));
				}
				/* Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j]))); */
				tmp = exp(-1.0 * mad_f_todouble(mad_f_sub(sum, Q_y[j])));
				mad_fixed_t exp_result = mad_f_tofixed(tmp);
				Y[j] = mad_f_div(F_POS_ONE, mad_f_add(F_POS_ONE, exp_result));
			}

			for (j = 0; j < num_out; j++) {
				/* delta_y[j] = Y[j] * (1.0 - Y[j]) * (T[j] - Y[j]); */
				mad_fixed_t first, second, third;
				first = Y[j];
				second = mad_f_sub(F_POS_ONE, Y[j]);
				third = mad_f_sub(T[j], Y[j]);
				delta_y[j] = mad_f_mul(mad_f_mul(first, second), third);
			}

			for (h = 0; h < num_hid; h++) {
				sum = F_ZERO;
				for (j = 0; j < num_out; j++) {
					/* sum = sum + W_hy[h][j] * delta_y[j]; */
					sum = mad_f_add(sum, mad_f_mul(W_hy[h][j], delta_y[j]));
				}
				/* delta_h[h] = H[h] * (1.0 - H[h]) * sum; */
				mad_fixed_t first, second, third;
				first = H[h];
				second = mad_f_sub(F_POS_ONE, H[h]);
				third = sum;
				delta_h[h] = mad_f_mul(mad_f_mul(first, second), sum);
			}

			for (j = 0; j < num_out; j++) {
				for (h = 0; h < num_hid; h++) {
					/* dW_hy[h][j] = eta * delta_y[j] * H[h] + alpha * dW_hy[h][j]; */
					mad_fixed_t first, second;
					first = mad_f_mul(mad_f_mul(F_ETA, delta_y[j]), H[h]);
					second = mad_f_mul(F_ALPHA, dW_hy[h][j]);
					dW_hy[h][j] = mad_f_add(first, second);
				}
			}

			for (j = 0; j < num_out; j++) {
				/* dQ_y[j] = -eta * delta_y[j] + alpha * dQ_y[j]; */
				mad_fixed_t first, second;
				first = mad_f_mul(F_NEG_ETA, delta_y[j]);
				second = mad_f_mul(F_ALPHA, dQ_y[j]);
				dQ_y[j] = mad_f_add(first, second);
			}
			for (h = 0; h < num_hid; h++) {
				for (i = 0; i < num_inp; i++) {
					/* dW_xh[i][h] = eta * delta_h[h] * X[i] + alpha * dW_xh[i][h]; */
					mad_fixed_t first, second;
					first = mad_f_mul(X[i], mad_f_mul(F_ETA, delta_h[h]));
					second = mad_f_mul(F_ALPHA, dW_xh[i][h]);
					dW_xh[i][h] = mad_f_add(first, second);
				}
			}

			for (h = 0; h < num_hid; h++) {
				/* dQ_h[h] = -eta * delta_h[h] + alpha * dQ_h[h]; */
				mad_fixed_t first, second;
				first = mad_f_mul(F_NEG_ETA, delta_h[h]);
				second = mad_f_mul(F_ALPHA, dQ_h[h]);
				dQ_h[h] = mad_f_add(first, second);
			}

			for (j = 0; j < num_out; j++) {
				for (h = 0; h < num_hid; h++) {
					/* W_hy[h][j] = W_hy[h][j] + dW_hy[h][j]; */
					tmp = mad_f_todouble(W_hy[h][j]);
					W_hy[h][j] = mad_f_add(W_hy[h][j], dW_hy[h][j]);
				}
			}

			for (j = 0; j < num_out; j++) {
				/* Q_y[j] = Q_y[j] + dQ_y[j]; */
				Q_y[j] = mad_f_add(Q_y[j], dQ_y[j]);
			}

			for (h = 0; h < num_hid; h++) {
				for (i = 0; i < num_inp; i++) {
					/* W_xh[i][h] = W_xh[i][h] + dW_xh[i][h]; */
					W_xh[i][h] = mad_f_add(W_xh[i][h], dW_xh[i][h]);
				}
			}

			for (h = 0; h < num_hid; h++) {
				/* Q_h[h] = Q_h[h] + dQ_h[h]; */
				Q_h[h] = mad_f_add(Q_h[h], dQ_h[h]);
			}

			for (j = 0; j < num_out; j++) {
				/* mse += (T[j] - Y[j]) * (T[j] - Y[j]); */
				mad_fixed_t first, second;
				first = mad_f_sub(T[j], Y[j]);
				second = mad_f_sub(T[j], Y[j]);
				mse = mad_f_add(mse, mad_f_mul(first, second));
			}
		}

		/* mse = mse / num_train; */
		mse = mad_f_div(mse, mad_f_tofixed(num_train));
		if ((Icycle % 10) == 9) {
			printf("\nIcycle=%3d \nmse=%-8.6f\n", Icycle, mad_f_todouble(mse));
			fprintf(fp3,"%3d \n%-8.6f\n", Icycle, mad_f_todouble(mse));
		}
	}

	printf("\n");
	for (h = 0; h < num_hid; h++) {
		for (i = 0; i < num_inp; i++) {
			printf("W_xh[%2d][%2d]=%-8.4f\t", i, h, mad_f_todouble(W_xh[i][h]));
			fprintf(fp2,"%-8.4f\t", mad_f_todouble(W_xh[i][h]));
		}
		printf("\n");
		fprintf(fp2,"\n");
	}
	printf("\n");
	fprintf(fp2,"\n");
	for (j = 0; j < num_out; j++) {
		for (h = 0; h < num_hid; h++) {
			printf("W_hy[%2d][%2d]=%-8.4f\t", h, j, mad_f_todouble(W_hy[h][j]));
			fprintf(fp2,"%-8.4f\t", mad_f_todouble(W_hy[h][j]));
		}
		printf("\n");
		fprintf(fp2,"\n");
	}
	printf("\n");
	fprintf(fp2,"\n");

	for (h = 0; h < num_hid; h++) {
		printf("Q_h[%2d]=%-8.4f\t", h, mad_f_todouble(Q_h[h]));
		fprintf(fp2,"%-8.4f\t", mad_f_todouble(Q_h[h]));
	}
	printf("\n\n");
	fprintf(fp2,"\n\n");
	for (j = 0; j < num_out; j++) {
		printf("Q_y[%2d]=%-8.4f\t", j, mad_f_todouble(Q_y[j]));
		fprintf(fp2,"%-8.4f\t", mad_f_todouble(Q_y[j]));
	}
	printf("\n\n");
	fprintf(fp2,"\n\n");

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	return 0;
}

