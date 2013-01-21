#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define		num_cycle		2000
#define		num_train  		1750
#define		num_inp			3
#define		num_hid    		4
#define		num_out    		1
#define		eta     		0.5
#define		alpha   		0.2
#define		train_file		"test.txt"
#define		weight_file		"pwm.wei"
#define		mse_file		"pwm.mes"

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

float random_value(void)
{
	return (rand()/(RAND_MAX+1.0))-0.5;
}

int main(int argc, char* argv[])
{
	FILE *fp1;
	FILE *fp2;
	FILE *fp3;
	float X[num_inp],T[num_out],H[num_hid],Y[num_out];
	float W_xh[num_inp][num_hid],W_hy[num_hid][num_out];
	float dW_xh[num_inp][num_hid],dW_hy[num_hid][num_out];
	float Q_h[num_hid],Q_y[num_out];
	float dQ_h[num_hid],dQ_y[num_out];
	float delta_h[num_hid],delta_y[num_out];
	float sum,mse;
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
			dW_xh[i][h] = 0;
		}
	}

	for (j = 0; j <= num_out; j++) {
		for (h = 0; h < num_hid; h++) {
			W_hy[h][j] = random_value();
			dW_hy[h][j] = 0;
		}
	}

	for (h = 0; h < num_hid; h++) {
		Q_h[h] = 0;
		dQ_h[h] = 0;
		delta_h[h] = 0;
	}

	for (j = 0; j < num_out; j++) {
		Q_y[j] = random_value();
		dQ_y[j] = 0;
		delta_y[j] = 0;
	}

	/*start learning */
	for (Icycle = 0; Icycle < num_cycle; Icycle++) {
		mse = 0.0;

		fseek(fp1, 0, 0);
		for (Itrain = 0; Itrain < num_train; Itrain++) {
			for (i = 0; i < num_inp; i++) {
				X[i] = parser(fp1);
			}

			for (j = 0; j < num_out; j++) {
				T[j]= parser(fp1);
			}

			for (h = 0; h < num_hid; h++) {
				sum = 0.0;
				for (i = 0; i < num_inp; i++) {
					sum = sum + X[i] * W_xh[i][h];
				}
				H[h] = (float)1.0 / (1.0 + exp(-(sum - Q_h[h])));
			}

			for (j = 0; j < num_out; j++) {
				sum = 0.0;
				for (h = 0; h < num_hid; h++)
					sum = sum + H[h] * W_hy[h][j];
				Y[j] = (float)1.0 / (1.0 + exp(-(sum - Q_y[j])));
			}

			for (j = 0; j < num_out; j++)
				delta_y[j] = Y[j] * (1.0 - Y[j]) * (T[j] - Y[j]);

			for (h = 0;h < num_hid; h++) {
				sum = 0.0;
				for (j = 0; j < num_out; j++)
					sum = sum + W_hy[h][j] * delta_y[j];
				delta_h[h] = H[h] * (1.0 - H[h]) * sum;
			}

			for (j = 0; j < num_out; j++) {
				for (h = 0; h < num_hid; h++) {
					float first, second;
					first = eta * delta_y[j] * H[h];
					second = alpha * dW_hy[h][j];
					dW_hy[h][j] = eta * delta_y[j] * H[h] + alpha * dW_hy[h][j];
				}
			}

			for (j = 0; j < num_out; j++)
				dQ_y[j] = -eta * delta_y[j] + alpha * dQ_y[j];
			for (h = 0; h < num_hid; h++)
				for (i = 0; i < num_inp; i++)
					dW_xh[i][h] = eta * delta_h[h] * X[i] + alpha * dW_xh[i][h];
			for (h = 0; h < num_hid; h++)
				dQ_h[h] = -eta * delta_h[h] + alpha * dQ_h[h];

			for (j = 0; j < num_out; j++) {
				for (h = 0; h < num_hid; h++) {
					W_hy[h][j] = W_hy[h][j] + dW_hy[h][j];
				}
			}

			for (j = 0; j < num_out; j++)
				Q_y[j] = Q_y[j] + dQ_y[j];

			for (h = 0; h < num_hid; h++) {
				for (i = 0; i < num_inp; i++) {
					W_xh[i][h] = W_xh[i][h] + dW_xh[i][h];
				}
			}

			for (h = 0; h < num_hid; h++)
				Q_h[h] = Q_h[h] + dQ_h[h];

			for (j = 0; j < num_out; j++)
				mse += (T[j] - Y[j]) * (T[j] - Y[j]);
		}

		/* mse = mse / num_train; */
		mse = mse / num_train;
		if ((Icycle%10) == 9) {
			printf("\nIcycle=%3d \nmse=%-8.4f\n",Icycle,mse);
			fprintf(fp3,"%3d \n%-8.4f\n",Icycle,mse);
		}
	}

	printf("\n");
	for (h = 0; h < num_hid; h++) {
		for (i = 0; i < num_inp; i++) {
			printf("W_xh[%2d][%2d]=%-8.2f",i,h,W_xh[i][h]);
			fprintf(fp2,"%-8.2f",W_xh[i][h]);
		}
		printf("\n");
		fprintf(fp2,"\n");
	}
	printf("\n");
	fprintf(fp2,"\n");
	for (j = 0; j < num_out; j++) {
		for (h = 0; h < num_hid; h++) {
			printf("W_hy[%2d][%2d]=%-8.2f",h,j,W_hy[h][j]);
			fprintf(fp2,"%-8.2f",W_hy[h][j]);
		}
		printf("\n");
		fprintf(fp2,"\n");
	}
	printf("\n");
	fprintf(fp2,"\n");

	for (h = 0; h < num_hid; h++) {
		printf("Q_h[%2d]=%-8.2f",h,Q_h[h]);
		fprintf(fp2,"%-8.2f",Q_h[h]);
	}
	printf("\n\n");
	fprintf(fp2,"\n\n");
	for (j = 0; j < num_out; j++) {
		printf("Q_y[%2d]=%-8.2f",j,Q_y[j]);
		fprintf(fp2,"%-8.2f",Q_y[j]);
	}
	printf("\n\n");
	fprintf(fp2,"\n\n");

	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	return 0;
}

