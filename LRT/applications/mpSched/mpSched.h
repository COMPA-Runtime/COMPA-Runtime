/****************************************************************************
 * Copyright or © or Copr. IETR/INSA (2013): Julien Heulot, Yaset Oliva,    *
 * Maxime Pelcat, Jean-François Nezan, Jean-Christophe Prevotet             *
 *                                                                          *
 * [jheulot,yoliva,mpelcat,jnezan,jprevote]@insa-rennes.fr                  *
 *                                                                          *
 * This software is a computer program whose purpose is to execute          *
 * parallel applications.                                                   *
 *                                                                          *
 * This software is governed by the CeCILL-C license under French law and   *
 * abiding by the rules of distribution of free software.  You can  use,    *
 * modify and/ or redistribute the software under the terms of the CeCILL-C *
 * license as circulated by CEA, CNRS and INRIA at the following URL        *
 * "http://www.cecill.info".                                                *
 *                                                                          *
 * As a counterpart to the access to the source code and  rights to copy,   *
 * modify and redistribute granted by the license, users are provided only  *
 * with a limited warranty  and the software's author,  the holder of the   *
 * economic rights,  and the successive licensors  have only  limited       *
 * liability.                                                               *
 *                                                                          *
 * In this respect, the user's attention is drawn to the risks associated   *
 * with loading,  using,  modifying and/or developing or reproducing the    *
 * software by the user in light of its specific status of free software,   *
 * that may mean  that it is complicated to manipulate,  and  that  also    *
 * therefore means  that it is reserved for developers  and  experienced    *
 * professionals having in-depth computer knowledge. Users are therefore    *
 * encouraged to load and test the software's suitability as regards their  *
 * requirements in conditions enabling the security of their systems and/or *
 * data to be ensured and,  more generally, to use and operate it in the    *
 * same conditions as regards security.                                     *
 *                                                                          *
 * The fact that you are presently reading this means that you have had     *
 * knowledge of the CeCILL-C license and that you accept its terms.         *
 ****************************************************************************/

#include <string.h>
#include <platform_types.h>

#include "actors.h"

//#define NVAL	20
//#define MMAX 	20
//#define NBITER 	10

//static UINT8 nValues[NBITER][MMAX] = {
//		{2,1},
//	};

void config(UINT8* inputFIFOs[],
			UINT8* outputFIFOs[],
			UINT32 params[])
{
//	UINT32 N;
	UINT32 NMAX = params[0];
	UINT32 N = params[1];
	int i;

//	N = NVAL;

//	printf("Recv N=%d\n", N);

	UINT8* out_M = outputFIFOs[0];

	// Sending parameter's value.
	platform_queue_push_UINT32(PlatformCtrlQueue, MSG_PARAM_VALUE);
	platform_queue_push_UINT32(PlatformCtrlQueue, OSTCBCur->vertexId);
	platform_queue_push_UINT32(PlatformCtrlQueue, N);
	platform_queue_push_finalize(PlatformCtrlQueue);

//	memcpy(out_M, nValues[0], NMAX);
	for(i=0; i<N; i++)
		out_M[i] = 8;//-i;
}


void mFilter(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT32 NMAX = params[0];
	UINT32 N = params[1];

	UINT8* in_m = inputFIFOs[0];
	UINT8* out_m = outputFIFOs[0];

	memcpy(out_m, in_m, N);
}
void src(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT32 i,j;
	UINT32 N = params[0];
	UINT32 NBSAMPLES = params[1];
	UINT32 TEST = params[2];

	float* out = (float*)outputFIFOs[0];

	if(TEST){
		for(i=0; i<N; i++){
			srand(1000);
			for(j=0; j<NBSAMPLES; j++){
				out[j+i*NBSAMPLES] = 10*((float)rand())/RAND_MAX;
			}
		}
	}
}

void snk(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT32 i,j;
	UINT32 N = params[0];
	UINT32 NBSAMPLES = params[1];
	UINT32 TEST = params[2];

#ifndef DSP
	const int expectedHash[13]={
		/*0 */ 0x1D8CCC7, 	/*1 */ 0x69D0FCD, 	/*2 */ 0x9CA48CA,
		/*3 */ 0x95CFC62, 	/*4 */ 0x5CAE39A, 	/*5 */ 0x170030E8,
		/*6 */ 0x16C43A1F,	/*7 */ 0x1F40E5E3,	/*8 */ 0x4D50F02B,
		/*9 */ 0x7D6D759, 	/*10*/ 0x49638F8,	/*11*/ 0x7BD1349F,
		/*12*/ 0x712A25F4
	};
#else
	const int expectedHash[13]={
		/*0 */ 0x06088D61,	/*1 */ 0x03902DCA,	/*2 */ 0x7BE6A549,
		/*3 */ 0x7D008129,	/*4 */ 0x152FE04C,	/*5 */ 0x61FA4D45,
		/*6 */ 0x762F3E52,	/*7 */ 0x1A0D6EA7,	/*8 */ 0x407294F4,
		/*9 */ 0x28182904,	/*10*/ 0x60F32492,	/*11*/ 0x630A18F6,
		/*12*/ 0x476BCCAB
	};
#endif

	float* in = (float*)inputFIFOs[0];

	BOOL test = TRUE;

	if(TEST){
		int hash;
		for(i=0; i<N; i++){
			hash = 0;
			int* data = (int*)in;
			for(j=0; j<NBSAMPLES; j++){
				hash = hash ^ data[j+i*NBSAMPLES];
			}
			if(hash != expectedHash[8]){
				printf("Bad Hash result: %#X instead of %#X\n", hash, expectedHash[8]);
				return;
			}
		}
		printf("Result OK\n");
	}
}

void setM(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT8* in_m = inputFIFOs[0];

	// Sending parameter's value.
	platform_queue_push_UINT32(PlatformCtrlQueue, MSG_PARAM_VALUE);
	platform_queue_push_UINT32(PlatformCtrlQueue, OSTCBCur->vertexId);
	platform_queue_push_UINT32(PlatformCtrlQueue, in_m[0]);
	platform_queue_push_finalize(PlatformCtrlQueue);


	//printf("Exec setM\n");
}

void initSwitch(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT32 M = params[0];
	UINT32 i;

	UINT8* out = outputFIFOs[0];

	out[0] = 0;
	for(i=1; i<M; i++){
		out[i] = 1;
	}
}

void switchFct(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT32 NBSAMPLES = params[0];

	UINT8 select = inputFIFOs[0][0];
	void *in0 = inputFIFOs[1];
//	void *in1 = inputFIFOs[2];
	void *out = outputFIFOs[0];

	if(select == 0){
		memcpy(out, in0, NBSAMPLES*sizeof(float));
	}else{
//		memcpy(out, in1, NBSAMPLES*sizeof(float));
	}
}


void FIR(UINT8* inputFIFOs[],
		UINT8* outputFIFOs[],
		UINT32 params[])
{
	UINT32 NBSAMPLES = params[0];

	float* in = (float*)inputFIFOs[0];
	float* out = (float*)outputFIFOs[0];

	fir(in, out, NBSAMPLES);
}


