/*
 *	Generated .c file from Newton
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "RandomAccelerationWalk-estSynth.h"

#include "matrix.h"
#include "matrixadv.h"

#define DEG2RAD (3.1415926535 / 180)

enum filterCoreStateIdx
{
	STATE_x_0,
	STATE_v_0,
	STATE_a_0,
	STATE_DIMENSION
};

enum filterMeasureIdx
{
	MEASURE_x_m_0,
	MEASURE_DIMENSION
};

typedef struct CoreState CoreState;
struct CoreState
{
	/*
	 *	State
	 */
	double S[STATE_DIMENSION];
	matrix *Sm;

	/*
	 *	State covariance matrix
	 */
	double P[STATE_DIMENSION][STATE_DIMENSION];
	matrix *Pm;

	/*
	 *	Process noise matrix
	 */
	double Q[STATE_DIMENSION][STATE_DIMENSION];
	matrix *Qm;

	/*
	 *	Process noise matrix
	 */
	double R[MEASURE_DIMENSION][MEASURE_DIMENSION];
	matrix *Rm;
};

void filterInit(CoreState *cState, double S0[STATE_DIMENSION], double P0[STATE_DIMENSION][STATE_DIMENSION])
{
	for (int i = STATE_x_0; i < STATE_DIMENSION; i++)
	{
		cState->S[i] = S0[i];
	}

	cState->Sm = makeMatrix(1, STATE_DIMENSION);
	cState->Sm->data = &cState->S[0];

	for (int i = STATE_x_0; i < STATE_DIMENSION; i++)
	{
		for (int j = STATE_x_0; j < STATE_DIMENSION; j++)
		{
			cState->P[i][j] = P0[i][j];
		}
	}

	cState->Pm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);
	cState->Pm->data = &cState->P[0][0];

	cState->Q[0][0] = 1e-06;
	cState->Q[0][1] = 1e-07;
	cState->Q[0][2] = 1e-07;
	cState->Q[1][0] = 1e-07;
	cState->Q[1][1] = 1e-06;
	cState->Q[1][2] = 1e-07;
	cState->Q[2][0] = 1e-07;
	cState->Q[2][1] = 1e-07;
	cState->Q[2][2] = 1e-06;

	cState->Qm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);
	cState->Qm->data = &cState->Q[0][0];

	cState->R[0][0] = 1e-06;

	cState->Rm = makeMatrix(MEASURE_DIMENSION, MEASURE_DIMENSION);
	cState->Rm->data = &cState->R[0][0];
}

void filterPredict(CoreState *cState, time dt)
{
	double fMatrix[STATE_DIMENSION][STATE_DIMENSION] =
	    {
		{
		    (1),
		    (1 * dt),
		    (0),
		},
		{
		    (0),
		    (1),
		    (1 * dt),
		},
		{
		    (0),
		    (0),
		    (1),
		},
	    };

	matrix Fm = {.height = STATE_DIMENSION, .width = STATE_DIMENSION, .data = &fMatrix[0][0]};
	matrix *FSm = multiplyMatrix(&Fm, cState->Sm);
	double *sn = FSm->data;
	double *s = &cState->S[0];

	for (int i = 0; i < STATE_DIMENSION; i++)
	{
		*s = *sn;
		s++;
		sn++;
	}

	matrix *Fm_T = transposeMatrix(&Fm);
	matrix *FPm = multiplyMatrix(&Fm, cState->Pm);
	matrix *FPFm_T = multiplyMatrix(FPm, Fm_T);

	double *p = cState->Pm->data;
	double *fpf = FPFm_T->data;
	double *q = cState->Qm->data;

	for (int i = 0; i < STATE_DIMENSION * STATE_DIMENSION; i++)
	{
		*p = *fpf + *q;
		p++;
		fpf++;
		q++;
	}
}

void filterUpdate(CoreState *cState, double Z[MEASURE_DIMENSION], time dt)
{
	double hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] =
	    {
		{
		    (1),
		    (0),
		    (0),
		},
	    };

	matrix Hm = {.height = MEASURE_DIMENSION, .width = STATE_DIMENSION, .data = &hMatrix[0][0]};
	matrix Zm = {.height = MEASURE_DIMENSION, .width = 1, .data = Z};

	// Kg = PH^T * (HPH^T + R)^(-1)
	matrix *Hm_T = transposeMatrix(&Hm);
	matrix *PHm_T = multiplyMatrix(cState->Pm, Hm_T);
	matrix *HPHm_T = multiplyMatrix(&Hm, PHm_T);

	double *hph = HPHm_T->data;
	double *r = cState->Rm->data;

	for (int i = 0; i < MEASURE_DIMENSION * MEASURE_DIMENSION; i++)
	{
		*hph += *r;
		hph++;
		r++;
	}

	matrix *HPHm_T_inv = inverseMatrix(HPHm_T);
	matrix *Kg = multiplyMatrix(PHm_T, HPHm_T_inv);

	// S <- S + Kg (Z - HS)
	matrix *HSm = multiplyMatrix(&Hm, cState->Sm);
	double *hs = HSm->data;
	double *z = &Z[0];

	for (int i = 0; i < MEASURE_DIMENSION; i++)
	{
		*hs = *z - *hs;
		hs++;
		z++;
	}

	matrix *KgZHS = multiplyMatrix(Kg, HSm);
	double *s = &cState->S[0];
	double *kgzhs = KgZHS->data;

	for (int i = 0; i < STATE_DIMENSION; i++)
	{
		*s += *kgzhs;
		s++;
		kgzhs++;
	}

	// P <- P - KgHP
	matrix *HPm = multiplyMatrix(&Hm, cState->Pm);
	matrix *KgHPm = multiplyMatrix(Kg, HPm);
	double *p = &cState->P[0][0];
	double *kghp = KgHPm->data;

	for (int i = 0; i < STATE_DIMENSION * STATE_DIMENSION; i++)
	{
		*p -= *kghp;
		p++;
		kghp++;
	}
}

int main(int argc, char *argv[])
{

	CoreState cs;
	double initState[STATE_DIMENSION];
	time timeElapsed = 0;
	initState[STATE_x_0] = (distance) 0;
	initState[STATE_v_0] = (speed) 0;
	initState[STATE_a_0] = (acceleration) 0;

	double initCov[STATE_DIMENSION][STATE_DIMENSION] = {
	    {
		100,
		1,
		1,
	    },
	    {
		1,
		100,
		1,
	    },
	    {
		1,
		1,
		100,
	    },
	};
	filterInit(&cs, initState, initCov);
	time dt;
	time prevtime = timeElapsed;
	distance measure[MEASURE_DIMENSION];
	while (scanf("%lf", &timeElapsed) > 0)
	{
		scanf(",%*lf");
		scanf(",%*lf");
		scanf(",%*lf");
		scanf(",%lf", &measure[0]);

		dt = timeElapsed - prevtime;

		filterPredict(&cs, dt);
		printf("Predict: %lf", timeElapsed);
		printf(", %lf", cs.S[STATE_x_0]);
		printf(", %lf", cs.S[STATE_v_0]);
		printf(", %lf", cs.S[STATE_a_0]);
		printf("\n");

		filterUpdate(&cs, measure, dt);
		printf("Update: %lf", timeElapsed);
		printf(", %lf", cs.S[STATE_x_0]);
		printf(", %lf", cs.S[STATE_v_0]);
		printf(", %lf", cs.S[STATE_a_0]);
		printf("\n");

		prevtime = timeElapsed;
	}

	return 0;
}

/*
 *	End of the generated .c file
 */
