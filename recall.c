#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define num_test	1
#define num_inp		3
#define num_hid		4
#define num_out		1
#define weight_file "pwm.wei"
#define test_file   "normal_test.txt"
#define recall_file "xor.rec"

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
	float	X[num_inp],T[num_out],H[num_hid],Y[num_out];
	float 	W_xh[num_inp][num_hid],W_hy[num_hid][num_out];
	float	Q_h[num_hid],Q_y[num_out];
	float	sum,mse;
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

	fseek(fp2, 0, 0);
	for (Itest = 0; Itest < num_test; Itest++) {
		for (i = 0; i < num_inp; i++) {
			X[i] = parser(fp2);
			printf("X[%d]=%f\t", i, X[i]);
		}
		printf("\n");

		for (j = 0; j < num_out; j++) {
			T[j] = parser(fp2);
		}

		for (h = 0; h < num_hid; h++) {
			sum = 0.0;
			for (i = 0; i < num_inp; i++) {
				sum = sum + X[i] * W_xh[i][h];
			}
			/* H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h]))); */
			H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h])));
			//printf("H[h]=%f\n", mad_f_todouble(H[h]));
		}

		for (j = 0; j < num_out; j++) {
			sum = 0.0;
			for (h = 0; h < num_hid; h++) {
				sum = sum + H[h] * W_hy[h][j];
			}
			/* Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j]))); */
			Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j])));
		}

		mse = 0.0;
		for (j = 0; j < num_out; j++) {
		 	/* mse += (T[j] - Y[j]) * (T[j] - Y[j]); */
			mse += (T[j] - Y[j]) * (T[j] - Y[j]);
		}

		printf("T[j]= ");
		fprintf(fp3, "T[j]= ");
		for (j = 0; j < num_out; j++) {
			printf("%-8.4f", T[j]);
			fprintf(fp3, "%-8.4f", T[j]);
		}
		printf("Y[j]= ");
		fprintf(fp3, "Y[j]= ");
		for (j = 0; j < num_out; j++) {
			printf("%-8.4f", Y[j]);
			fprintf(fp3,"%-8.4f", Y[j]);
		}
		printf("  mse= %-8.6f\n\n", mse);
		fprintf(fp3, "  mse= %-8.6f", mse);
		fprintf(fp3, "\n");
	}

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);

	return 0;
}

