/*
 * main.c
 *
 *  Created on: 13 sept. 2013
 *      Author: yoliva
 */

#include <stdio.h>
#include <stdlib.h>
#include <sharedMem.h>
#include <hwQueues.h>
#include <lrt_1W1RfifoMngr.h>
#include <platform.h>
#include <lrt_core.h>
#include <lrt_taskMngr.h>

//#include "definitions.h"
//#include "top_AM_actions.h"

#define STANDALONE_APP		1



void readVOL(UINT32 inputFIFOIds[],
			 UINT32 inputFIFOAddrs[],
			 UINT32 outputFIFOIds[],
			 UINT32 outputFIFOAddrs[],
			 UINT32 params[])
{
	UINT8 inData;
	readFifo(inputFIFOIds[0],inputFIFOAddrs[0], sizeof(inData), &inData);

	/* Sending data */
	writeFifo(outputFIFOIds[0], outputFIFOAddrs[0], sizeof(inData), &inData);
	writeFifo(outputFIFOIds[1], outputFIFOAddrs[1], sizeof(inData), &inData);
	writeFifo(outputFIFOIds[2], outputFIFOAddrs[2], sizeof(inData), &inData);
	writeFifo(outputFIFOIds[3], outputFIFOAddrs[3], sizeof(inData), &inData);
	writeFifo(outputFIFOIds[4], outputFIFOAddrs[4], sizeof(inData), &inData);
}


void decodeDsply(UINT32 inputFIFOIds[],
		 UINT32 inputFIFOAddrs[],
		 UINT32 outputFIFOIds[],
		 UINT32 outputFIFOAddrs[],
		 UINT32 params[])
{

	UINT8 inData;
	readFifo(inputFIFOIds[0],inputFIFOAddrs[0], sizeof(inData), &inData);
	printf("%d\n", inData);
	readFifo(inputFIFOIds[1],inputFIFOAddrs[1], sizeof(inData), &inData);
	printf("%d\n", inData);
	readFifo(inputFIFOIds[2],inputFIFOAddrs[2], sizeof(inData), &inData);
	printf("%d\n", inData);
	readFifo(inputFIFOIds[3],inputFIFOAddrs[3], sizeof(inData), &inData);
	printf("%d\n", inData);
}



int main(int argc, char **argv) {
//	if(argc > 1)
//		cpuId = atoi(argv[1]);
//	else{
//		printf("You must specify procID\n");
//		return -1;
//	}
	cpuId = 0;

	// Top L0.
	functions_tbl[0] = readVOL;
	functions_tbl[1] = decodeDsply;

#if STANDALONE_APP == 0
	LRTInit();
#else
	printf("Standalone application..\n");
	OSTCBCur = (OS_TCB *) 0;
	lrt_running = FALSE;
	OS_ShMemInit();
	flushFIFO(-1);	// Clear all FIFOs.
	// Creating tasks and starting executions.

	// Testing FIFOs.
	UINT32 inputFIFOIds_readVOL[1] = {0};
	UINT32 inputFIFOAddrs_readVOL[1] = {0};

	UINT32 outputFIFOIds_readVOL[5] = {1, 2, 3, 4, 0};
	UINT32 outputFIFOAddrs_readVOL[5] = {1024, 2048, 3072, 4096, 0};

	UINT32 inputFIFOIds_decodeDsply[4] = {1, 2, 3, 4};
	UINT32 inputFIFOAddrs_decodeDsply[4] = {1024, 2048, 3072, 4096};

	readVOL(inputFIFOIds_readVOL, inputFIFOAddrs_readVOL, outputFIFOIds_readVOL, outputFIFOAddrs_readVOL, 0);
	decodeDsply(inputFIFOIds_decodeDsply, inputFIFOAddrs_decodeDsply, 0, 0, 0);
#endif

	return 0;
}
