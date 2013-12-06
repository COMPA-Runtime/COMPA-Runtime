/****************************************************************************
 * Copyright or � or Copr. IETR/INSA (2013): Julien Heulot, Yaset Oliva,	*
 * Maxime Pelcat, Jean-Fran�ois Nezan, Jean-Christophe Prevotet				*
 * 																			*
 * [jheulot,yoliva,mpelcat,jnezan,jprevote]@insa-rennes.fr					*
 * 																			*
 * This software is a computer program whose purpose is to execute			*
 * parallel applications.													*
 * 																			*
 * This software is governed by the CeCILL-C license under French law and	*
 * abiding by the rules of distribution of free software.  You can  use, 	*
 * modify and/ or redistribute the software under the terms of the CeCILL-C	*
 * license as circulated by CEA, CNRS and INRIA at the following URL		*
 * "http://www.cecill.info". 												*
 * 																			*
 * As a counterpart to the access to the source code and  rights to copy,	*
 * modify and redistribute granted by the license, users are provided only	*
 * with a limited warranty  and the software's author,  the holder of the	*
 * economic rights,  and the successive licensors  have only  limited		*
 * liability. 																*
 * 																			*
 * In this respect, the user's attention is drawn to the risks associated	*
 * with loading,  using,  modifying and/or developing or reproducing the	*
 * software by the user in light of its specific status of free software,	*
 * that may mean  that it is complicated to manipulate,  and  that  also	*
 * therefore means  that it is reserved for developers  and  experienced	*
 * professionals having in-depth computer knowledge. Users are therefore	*
 * encouraged to load and test the software's suitability as regards their	*
 * requirements in conditions enabling the security of their systems and/or *
 * data to be ensured and,  more generally, to use and operate it in the 	*
 * same conditions as regards security. 									*
 * 																			*
 * The fact that you are presently reading this means that you have had		*
 * knowledge of the CeCILL-C license and that you accept its terms.			*
 ****************************************************************************/

#include "LRTActor.h"
#include "Memory.h"

LRTActor::LRTActor(SRDAGGraph *graph, SRDAGVertex* srvertex, launcher* curLaunch){
	this->nbInputFifos = srvertex->getNbInputEdge();
	this->nbOutputFifos = srvertex->getNbOutputEdge();

	for(UINT32 i=0; i<this->nbInputFifos; i++){
		SRDAGEdge* edge = srvertex->getInputEdge(i);
		this->inFIFOs[i] = curLaunch->getFIFO(graph->getEdgeIndex(edge));
//		this->inputFifoId[i] = graph->getEdgeIndex(edge);
//		this->readDataSize[i] = edge->getTokenRate() * DEFAULT_FIFO_SIZE; // TODO: the size should come within the edge.
//		this->inputFifoAddr[i] = mem->alloc(this->readDataSize[i]);
	}

	for(UINT32 i=0; i<this->nbOutputFifos; i++){
		SRDAGEdge* edge = srvertex->getOutputEdge(i);
		this->outFIFOs[i] = curLaunch->getFIFO(graph->getEdgeIndex(edge));
//		this->outputFifoId[i] = graph->getEdgeIndex(edge);
//		this->writeDataSize[i] = edge->getTokenRate() * DEFAULT_FIFO_SIZE; // TODO: the size should come within the edge.
//		this->outputFifoAddr[i] = mem->alloc(this->writeDataSize[i]);
	}
}


void LRTActor::prepare(int slave, launcher* launch){
//	launch->addUINT32ToSend(slave, MSG_CREATE_JOB);
//	launch->addUINT32ToSend(slave, this->functID);
	launch->addUINT32ToSend(slave, this->nbInputFifos);
	launch->addUINT32ToSend(slave, this->nbOutputFifos);
	for (UINT32 i = 0; i < this->nbInputFifos; i++) {
		launch->addUINT32ToSend(slave, this->inFIFOs[i]->id);
		launch->addUINT32ToSend(slave, this->inFIFOs[i]->addr);
		// TODO: see if the FIFO' size is required.
//		launch->addUINT32ToSend(slave, this->inputFifoId[i]);
//		launch->addUINT32ToSend(slave, this->inputFifoAddr[i]);

	}
	for (UINT32 i = 0; i < this->nbOutputFifos; i++) {
		launch->addUINT32ToSend(slave, this->outFIFOs[i]->id);
		launch->addUINT32ToSend(slave, this->outFIFOs[i]->addr);
		// TODO: see if the FIFO' size is required.
//		launch->addUINT32ToSend(slave, this->outputFifoId[i]);
//		launch->addUINT32ToSend(slave, this->outputFifoAddr[i]);
	}
	// TODO:
//	for(int j=0; j<action->getNbArgs(); j++)
//		launch->addUINT32ToSend(slave, action->getArg(j));

//	launch->addUINT32ToReceive(slave, MSG_CREATE_TASK);
}
