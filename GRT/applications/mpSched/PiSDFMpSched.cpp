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

#include "PiSDFMpSched.h"

void mpSched(PiSDFGraph* graph, int NMAX);
void mpSched_sub(PiSDFGraph* graph);


static PiSDFGraph* graphs;
static int nbGraphs = 0;

PiSDFGraph* addGraph(){
	if(nbGraphs >= MAX_NB_PiSDF_GRAPHS) exitWithCode(1054);
	PiSDFGraph* graph = &(graphs[nbGraphs++]);
	graph->reset();
	return graph;
}

PiSDFGraph* initPisdf_mpSched(PiSDFGraph* _graphs, int NMAX){
	graphs = _graphs;

	PiSDFGraph* top = addGraph();
	top->setBaseId(0);

	PiSDFVertex *vxTop = (PiSDFVertex *)top->addVertex("top", pisdf_vertex);
	top->setRootVertex(vxTop);

	PiSDFGraph* mpSchedGraph = addGraph();
	mpSchedGraph->setBaseId(10);
	vxTop->setSubGraph(mpSchedGraph);
	mpSchedGraph->setParentVertex(vxTop);

	mpSched(mpSchedGraph, NMAX);

	return top;
}

void mpSched(PiSDFGraph* graph, int NMAX){
	// Parameters.
	PiSDFParameter *paramN = graph->addParameter("N");
	PiSDFParameter *paramNMAX = graph->addParameter("NMAX");

	paramNMAX->setValue(NMAX);

#if EXEC == 0
	paramN->setValue(2);
#endif

	// Configure vertices.
	PiSDFConfigVertex *vxConfig = (PiSDFConfigVertex *)graph->addVertex("config", config_vertex);
	vxConfig->setFunction_index(0);
	vxConfig->addParameter(paramNMAX);
	vxConfig->addRelatedParam(paramN);
	graph->setRootVertex(vxConfig);

	// Other vertices
	PiSDFVertex *vxMFilter 	= (PiSDFVertex *)graph->addVertex("MFilter", pisdf_vertex);
	vxMFilter->addParameter(paramNMAX);
	vxMFilter->addParameter(paramN);
	vxMFilter->setFunction_index(1);

	PiSDFVertex *vxSrc = (PiSDFVertex *)graph->addVertex("Src", pisdf_vertex);
	vxSrc->addParameter(paramN);
	vxMFilter->setFunction_index(2);

	PiSDFVertex *vxSnk 	= (PiSDFVertex *)graph->addVertex("Snk", pisdf_vertex);
	vxSnk->addParameter(paramN);
	vxSnk->setFunction_index(3);

	PiSDFVertex *vxUserFIRs	= (PiSDFVertex *)graph->addVertex("UserFIRs", pisdf_vertex);

	// Edges.
	graph->addEdge(vxConfig, 0, "NMAX", vxMFilter, 0, "NMAX", "0");
	graph->addEdge(vxMFilter, 0, "N", vxUserFIRs, 0, "1", "0");

	graph->addEdge(vxSrc, 0, "N", vxUserFIRs, 1, "1", "0");
	graph->addEdge(vxUserFIRs, 0, "1", vxSnk, 0, "N", "0");

	// Timings
	vxConfig->setTiming(0, "1");
	vxMFilter->setTiming(0, "1");
	vxSrc->setTiming(0, "1");
	vxSnk->setTiming(0, "1");

	// Subgraphs
	PiSDFGraph *MpSched_subGraph = addGraph();
	MpSched_subGraph->setBaseId(20);
	vxUserFIRs->setSubGraph(MpSched_subGraph);
	MpSched_subGraph->setParentVertex(vxUserFIRs);

	mpSched_sub(MpSched_subGraph);
}

void mpSched_sub(PiSDFGraph* graph){
	// Parameters.
	PiSDFParameter *paramM = graph->addParameter("M");

#if EXEC == 0
		paramM->setValue(2);
#endif

	// Interface vertices.
	PiSDFIfVertex *vxM = (PiSDFIfVertex*)graph->addVertex("M", input_vertex);
	vxM->setDirection(0);
	vxM->setParentVertex(graph->getParentVertex());
	vxM->setParentEdge(graph->getParentVertex()->getInputEdge(0));
	vxM->setFunction_index(10);

	PiSDFIfVertex *vxIn = (PiSDFIfVertex*)graph->addVertex("In", input_vertex);
	vxIn->setDirection(0);
	vxIn->setParentVertex(graph->getParentVertex());
	vxIn->setParentEdge(graph->getParentVertex()->getInputEdge(0));
	vxIn->setFunction_index(10);

	PiSDFIfVertex *vxOut = (PiSDFIfVertex*)graph->addVertex("Out", output_vertex);
	vxOut->setDirection(1);
	vxOut->setParentVertex(graph->getParentVertex());
	vxOut->setParentEdge(graph->getParentVertex()->getOutputEdge(0));
	vxOut->setFunction_index(10);

	// Configure vertices.
	PiSDFConfigVertex *vxSetM = (PiSDFConfigVertex *)graph->addVertex("setM", config_vertex);
	vxSetM->setFunction_index(4);
	vxSetM->addRelatedParam(paramM);
	graph->setRootVertex(vxSetM);

	// Other vertices
	PiSDFVertex *vxInit 	= (PiSDFVertex *)graph->addVertex("Init", pisdf_vertex);
	vxInit->addParameter(paramM);
	vxInit->setFunction_index(5);

	PiSDFVertex *vxSwitch 	= (PiSDFVertex *)graph->addVertex("Switch", pisdf_vertex);
	vxSwitch->setFunction_index(6);

	PiSDFVertex *vxFIR	= (PiSDFVertex *)graph->addVertex("FIR", pisdf_vertex);
	vxFIR->setFunction_index(7);

	PiSDFVertex *vxBr	= (PiSDFVertex *)graph->addVertex("BroadCast", pisdf_vertex);
	vxBr->setFunction_index(11);

	// Edges.
	graph->addEdge(vxM, 0, "1", vxSetM, 0, "1", "0");
	graph->addEdge(vxInit, 0, "M", vxSwitch, 0, "1", "0");
	graph->addEdge(vxIn, 0, "1", vxSwitch, 1, "1", "0");
	graph->addEdge(vxBr, 1, "1", vxSwitch, 2, "1", "1");
	graph->addEdge(vxSwitch, 0, "1", vxFIR, 0, "1", "0");
	graph->addEdge(vxFIR, 0, "1", vxBr, 0, "1", "0");
	graph->addEdge(vxBr, 0, "1", vxOut, 0, "1", "0");

	// Timings
	vxSetM->setTiming(0, "1");
	vxInit->setTiming(0, "1");
	vxSwitch->setTiming(0, "1");
	vxFIR->setTiming(0, "1000");
	vxBr->setTiming(0, "1");
}
