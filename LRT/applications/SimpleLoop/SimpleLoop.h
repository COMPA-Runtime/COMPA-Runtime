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

#define MAX(a,b) (a>b?a:b)

void rdFile(FIFO inputFIFOs[],
			FIFO outputFIFOs[],
			UINT32 params[])
{
	UINT8 N, i;//, j;
	UINT32 NMAX;
	UINT8* array;

	NMAX = params[0];

	array = OSAllocWorkingMemory(NMAX);

	N = NMAX;
	for(i=0; i<NMAX; ++i){
		array[i] = NMAX+i;
	}

	// Sending parameter's value.
	platform_queue_push_UINT32(PlatformCtrlQueue, MSG_PARAM_VALUE);
	platform_queue_push_UINT32(PlatformCtrlQueue, OSTCBCur->vertexId);
	platform_queue_push_UINT32(PlatformCtrlQueue, N);
	platform_queue_push_finalize(PlatformCtrlQueue);

	platform_writeFifo(outputFIFOs[0].id, outputFIFOs[0].add, NMAX * sizeof(UINT8), array);
}


void initNLoop(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
//	UINT32 i;
	UINT32 N,NMAX;
	UINT8 *array;

	N = params[0];
	NMAX = params[1];

	array = OSAllocWorkingMemory(NMAX);

	platform_readFifo(inputFIFOs[0].id,inputFIFOs[0].add, NMAX * sizeof(UINT8), array);

//	printf("Init N loop with N=%d M = %d", N, M[0]);
//	for (i = 1; i < N; i++) {
//		printf(", %d", M[i]);
//	}
//	printf("\n");

	platform_writeFifo(outputFIFOs[0].id,outputFIFOs[0].add, N * sizeof(UINT8), array);
}


void endNLoop(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
	UINT32 N,NMAX;
	UINT8 *array;

	N = params[0];
	NMAX = params[1];

	array = OSAllocWorkingMemory(NMAX);

	platform_readFifo(inputFIFOs[0].id,inputFIFOs[0].add, N * sizeof(UINT8), array);
	memset(array+N,0,(NMAX-N));

	platform_writeFifo(outputFIFOs[0].id,outputFIFOs[0].add, NMAX * sizeof(UINT8), array);
}


void wrFile(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
	UINT32 NMAX;
	UINT8 *array;

	NMAX = params[0];

	array = OSAllocWorkingMemory(NMAX);

	platform_readFifo(inputFIFOs[0].id, inputFIFOs[0].add, NMAX * sizeof(UINT8), array);

	int i;
	for(i=0; i<NMAX; ++i){
		if(array[i] != (unsigned char)(NMAX+i+1)){
			printf("Nb(%d): Error bad value\n", NMAX);
			printf("(%d)(%d,%d) expected (%d) get (%d)\n", i, i/NMAX, i%NMAX, NMAX+i+1, array[i]);
			abort();
		}
	}
}



void f(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
	UINT8 inData;
	UINT8 outData;

	platform_readFifo(inputFIFOs[0].id, inputFIFOs[0].add, sizeof(UINT8), &inData);

	outData = ++inData;

	platform_writeFifo(outputFIFOs[0].id,outputFIFOs[0].add, sizeof(UINT8), &outData);
//	printf("F -> %d\n", outputFIFOs[0].id);
}


//******** Special actors ***********//
void RB(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
	UINT32 nbTknIn, nbTknOut;//, i;
	UINT32 quotient, residual;
	UINT8 *data;


	nbTknIn = params[0];
	nbTknOut = params[1];

	data = OSAllocWorkingMemory(MAX(nbTknIn,nbTknOut));

	platform_readFifo(inputFIFOs[0].id,inputFIFOs[0].add, nbTknIn * sizeof(UINT8), data);


//	printf("Round buffering %d to %d: %d", nbTknIn, nbTknOut, data[0]);
//	for (i = 1; i < nbTknIn; i++) {
//		printf(", %d", data[i]);
//	}
//	printf("\n");

	if(nbTknIn == nbTknOut){
		platform_writeFifo(outputFIFOs[0].id, outputFIFOs[0].add, nbTknIn * sizeof(UINT8), data);
	}
	else if(nbTknIn < nbTknOut){
		quotient = nbTknOut / nbTknIn;
		residual = nbTknOut % nbTknIn;
		platform_writeFifo(outputFIFOs[0].id, outputFIFOs[0].add, quotient * sizeof(UINT8), data);
		if(residual > 0)
			platform_writeFifo(outputFIFOs[0].id, outputFIFOs[0].add, residual * sizeof(UINT8), data);
	}
	else{
		printf("Error in RB, incoming tokens > outgoing tokens\n");
		exit(-1);
	}
}

void broadcast(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
	UINT32 nbFifoOut, nbTknIn, i;
	UINT8 *data;

	nbFifoOut = params[0];
	nbTknIn = params[1];

	data = OSAllocWorkingMemory(nbTknIn);

	platform_readFifo(inputFIFOs[0].id,inputFIFOs[0].add, nbTknIn * sizeof(UINT8), data);

//	printf("Broadcasting: %d", data[0]);
//	for (i = 1; i < nbTknIn; i++) {
//		printf(", %d", data[i]);
//	}
//	printf("\n");

	for (i = 0; i < nbFifoOut; i++) {
		platform_writeFifo(outputFIFOs[i].id, outputFIFOs[i].add, nbTknIn * sizeof(UINT8), data);
	}
}


void Xplode(FIFO inputFIFOs[],
		FIFO outputFIFOs[],
		UINT32 params[])
{
	UINT32 nbFifoIn, nbFifoOut, i, index;
	UINT8 *data;
//	UINT32 nbTknIn, nbTknOut;
	UINT32 inFifoBase, outFifoBase;

	nbFifoIn = params[0];
	inFifoBase = params[1];
	nbFifoOut = params[2];
	outFifoBase = params[3];

	for(i=0; i<nbFifoIn; i++){
		platform_readFifo(inFifoBase+i, 0, 0, 0);
	}
	for(i=0; i<nbFifoOut; i++){
		platform_writeFifo(outFifoBase+i, 0, 0, 0);
	}
//	index = 0;
//	if(nbFifoIn == 1){
//		/* Explode */
//		nbTknIn = params[2];
////		data = OSAllocWorkingMemory(nbTknIn);
//
//		platform_readFifo(inputFIFOs[0].id,inputFIFOs[0].add, 0, 0);
//
////		printf("Exploding : %d", data[0]);
////		for (i = 1; i < nbTknIn; i++) {
////			printf(", %d", data[i]);
////		}
////		printf(" -> ");
//
//		for(i=0; i<nbFifoOut; i++){
//			nbTknOut = params[i + 3];
////			printf(" {%d tkn}", nbTknOut);
//			platform_writeFifo(outputFIFOs[i].id, outputFIFOs[i].add, 0, 0);
//			index += nbTknOut;
//		}
////		printf("\n");
//	}else if(nbFifoOut == 1){
//		/* Implode */
////		printf("Imploding : ");
//		nbTknOut = params[nbFifoIn + 2];
////		data = OSAllocWorkingMemory(nbTknOut);
//
//		for(i=0; i<nbFifoIn; i++){
//			nbTknIn = params[i + 2];
////			printf("{%d tkn}", nbTknIn);
//			platform_readFifo(inputFIFOs[i].id, inputFIFOs[i].add, 0, 0);
//			index += nbTknIn;
//		}
////		printf(" -> %d", data[0]);
////		for(i=1; i<nbTknOut; i++){
////			printf(", %d", data[i]);
////
////		}
////		printf("\n");
//		platform_writeFifo(outputFIFOs[0].id,outputFIFOs[0].add, 0, data);
//	}else{
//		printf("Error in Xplode\n");
//		exit(-1);
//	}
}
