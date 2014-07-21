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

#ifndef SRDAG_VERTEX_CONFIG
#define SRDAG_VERTEX_CONFIG

class SRDAGGraph;
#include <cstring>

#include <grt_definitions.h>
#include "../../tools/SchedulingError.h"
#include "../PiSDF/PiSDFAbstractVertex.h"
#include "../PiSDF/PiSDFVertex.h"
#include "SRDAGEdge.h"
#include "SRDAGVertexAbstract.h"
#include <tools/IndexedArray.h>
#include <graphs/PiSDF/PiSDFConfigVertex.h>

class SRDAGVertexConfig : public SRDAGVertexAbstract{

private :
	IndexedArray<SRDAGEdge*, MAX_SRDAG_IO_EDGES> outputEdges;
	IndexedArray<SRDAGEdge*, MAX_SRDAG_IO_EDGES> inputEdges;

	int relatedParamValues[MAX_PARAM];

	int paramValues[MAX_PARAM];
	UINT32 execTime[MAX_SLAVE_TYPES];
	bool constraints[MAX_SLAVE_TYPES];

	void connectInputEdge(SRDAGEdge* edge, int ix);
	void connectOutputEdge(SRDAGEdge* edge, int ix);
	void disconnectInputEdge(int ix);
	void disconnectOutputEdge(int ix);

public :
	SRDAGVertexConfig(){}
	SRDAGVertexConfig(
					SRDAGGraph* 	_graph,
					int 			_refIx,
					int 			_itrIx,
					PiSDFConfigVertex* ref);
	~SRDAGVertexConfig(){}

	int getNbInputEdge() const;
	int getNbOutputEdge() const;
	SRDAGEdge* getInputEdge(int id);
	SRDAGEdge* getOutputEdge(int id);

	int getParamNb() const;
	int getParamValue(int paramIndex);
	UINT32 getExecTime(int slaveType) const;
	bool getConstraint(int slaveType) const;

	int getRelatedParamValue(int paramIndex) const;
	void setRelatedParamValue(int paramIndex, int value);

	virtual int getFctIx() const;

	BOOL isHierarchical() const;
	PiSDFGraph* getHierarchy() const;

	void getName(char* name, UINT32 sizeMax);
};

inline int SRDAGVertexConfig::getNbInputEdge() const
	{return inputEdges.getNb();}

inline int SRDAGVertexConfig::getNbOutputEdge() const
	{return outputEdges.getNb();}

inline SRDAGEdge* SRDAGVertexConfig::getInputEdge(int id)
	{return inputEdges[id];}

inline SRDAGEdge* SRDAGVertexConfig::getOutputEdge(int id)
	{return outputEdges[id];}

inline void SRDAGVertexConfig::connectInputEdge(SRDAGEdge* edge, int ix)
	{inputEdges.setValue(ix, edge);}

inline void SRDAGVertexConfig::connectOutputEdge(SRDAGEdge* edge, int ix)
	{outputEdges.setValue(ix, edge);}

inline void SRDAGVertexConfig::disconnectInputEdge(int ix)
	{inputEdges.resetValue(ix);}

inline void SRDAGVertexConfig::disconnectOutputEdge(int ix)
	{outputEdges.resetValue(ix);}

inline int SRDAGVertexConfig::getParamNb() const
	{return reference->getNbParameters();}

inline int SRDAGVertexConfig::getParamValue(int paramIndex)
	{return paramValues[paramIndex];}

inline UINT32 SRDAGVertexConfig::getExecTime(int slaveType) const
	{return execTime[slaveType];}

inline bool SRDAGVertexConfig::getConstraint(int slaveType) const
	{return constraints[slaveType];}

inline int SRDAGVertexConfig::getRelatedParamValue(int paramIndex) const
	{return relatedParamValues[paramIndex];}

inline void SRDAGVertexConfig::setRelatedParamValue(int paramIndex, int value)
	{relatedParamValues[paramIndex] = value;}

inline int SRDAGVertexConfig::getFctIx() const
	{return reference->getFunction_index();}

inline BOOL SRDAGVertexConfig::isHierarchical() const{
	return reference
				&& reference->getType() == normal_vertex
				&& ((PiSDFVertex*)reference)->hasSubGraph()
				&& type == Normal;
}

inline PiSDFGraph* SRDAGVertexConfig::getHierarchy() const
	{return ((PiSDFVertex*)reference)->getSubGraph();}

inline void SRDAGVertexConfig::getName(char* name, UINT32 sizeMax){
	int len = snprintf(name,MAX_VERTEX_NAME_SIZE,"%s_%d_%d",reference->getName(),itrIx, refIx);
	if(len > MAX_VERTEX_NAME_SIZE)
		exitWithCode(1075);
}

#endif