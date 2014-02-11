
/********************************************************************************
 * Copyright or � or Copr. IETR/INSA (2013): Julien Heulot, Yaset Oliva,	*
 * Maxime Pelcat, Jean-Fran�ois Nezan, Jean-Christophe Prevotet			*
 * 										*
 * [jheulot,yoliva,mpelcat,jnezan,jprevote]@insa-rennes.fr			*
 * 										*
 * This software is a computer program whose purpose is to execute		*
 * parallel applications.							*
 * 										*
 * This software is governed by the CeCILL-C license under French law and	*
 * abiding by the rules of distribution of free software.  You can  use, 	*
 * modify and/ or redistribute the software under the terms of the CeCILL-C	*
 * license as circulated by CEA, CNRS and INRIA at the following URL		*
 * "http://www.cecill.info". 							*
 * 										*
 * As a counterpart to the access to the source code and  rights to copy,	*
 * modify and redistribute granted by the license, users are provided only	*
 * with a limited warranty  and the software's author,  the holder of the	*
 * economic rights,  and the successive licensors  have only  limited		*
 * liability. 									*
 * 										*
 * In this respect, the user's attention is drawn to the risks associated	*
 * with loading,  using,  modifying and/or developing or reproducing the	*
 * software by the user in light of its specific status of free software,	*
 * that may mean  that it is complicated to manipulate,  and  that  also	*
 * therefore means  that it is reserved for developers  and  experienced	*
 * professionals having in-depth computer knowledge. Users are therefore	*
 * encouraged to load and test the software's suitability as regards their	*
 * requirements in conditions enabling the security of their systems and/or 	*
 * data to be ensured and,  more generally, to use and operate it in the 	*
 * same conditions as regards security. 					*
 * 										*
 * The fact that you are presently reading this means that you have had		*
 * knowledge of the CeCILL-C license and that you accept its terms.		*
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "types.h"
#include "swfifoMngr.h"
#include "sharedMem.h"
#include "hwQueues.h"
#include <grt_definitions.h>

#define NB_MAX_QUEUES				60
#define QUEUE_SIZE					512
#define INPUT_CTRL_QUEUE_MEM_BASE	0x20000000
#define OUTPUT_CTRL_QUEUE_MEM_BASE	0x20000200


static RT_SW_FIFO_HNDLE RTQueue[MAX_SLAVES][nbQueueTypes][2];

void RTQueuesInit(UINT8 nbSlaves){
	ShMemInit();
	for (int i = 0; i < nbSlaves; i++) {
		// Creating input queues.
		create_swfifo(&(RTQueue[i][RTCtrlQueue][RTInputQueue]), QUEUE_SIZE, INPUT_CTRL_QUEUE_MEM_BASE + i * (2 * QUEUE_SIZE));
		flush_swfifo(&(RTQueue[i][RTCtrlQueue][RTInputQueue]));

		// Creating output queues.
		create_swfifo(&(RTQueue[i][RTCtrlQueue][RTOutputQueue]), QUEUE_SIZE, OUTPUT_CTRL_QUEUE_MEM_BASE + i * (2 * QUEUE_SIZE));
		flush_swfifo(&(RTQueue[i][RTCtrlQueue][RTOutputQueue]));
	}
}


UINT32 RTQueuePush(UINT8 slaveId, RTQueueType queueType, void* data, int size){
	write_output_swfifo(&RTQueue[slaveId][queueType][RTOutputQueue], size, (UINT8*)data);
	return size;
}


UINT32 RTQueuePush_UINT32(UINT8 slaveId, RTQueueType queueType, UINT32 data){
	return RTQueuePush(slaveId, queueType, &data, sizeof(UINT32));
}


UINT32 RTQueuePop(UINT8 slaveId, RTQueueType queueType, void* data, int size){
	read_input_swfifo(&RTQueue[slaveId][queueType][RTInputQueue], size, (UINT8*)data);
	return size;
}


UINT32 RTQueuePop_UINT32(UINT8 slaveId, RTQueueType queueType){
	UINT32 data;
	return RTQueuePop(slaveId, queueType, &data, 4);
}


UINT32 RTQueueNonBlockingPop(UINT8 slaveId, RTQueueType queueType, void* data, int size){
	if(check_input_swfifo(&RTQueue[slaveId][queueType][RTInputQueue], size)){
		read_input_swfifo(&RTQueue[slaveId][queueType][RTInputQueue], size, (UINT8*)data);
		return size;
	}
	else
		return 0;
}
