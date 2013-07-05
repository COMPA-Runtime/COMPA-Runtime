/*
 * PiCSDFGraph.cpp
 *
 *  Created on: 12 juin 2013
 *      Author: yoliva
 */

#include "PiCSDFGraph.h"


/**
 Adding an edge to the graph. Vertices and edges must be added in topological order.

 @param source: The source vertex of the edge
 @param production: number of tokens (chars) produced by the source
 @param sink: The sink vertex of the edge
 @param consumption: number of tokens (chars) consumed by the sink
 @param initial_tokens: number of initial tokens.
 @return the created edge
*/
PiCSDFEdge* PiCSDFGraph::addEdge(CSDAGVertex* source, const char* production, CSDAGVertex* sink, const char* consumption, const char* delay){
	PiCSDFEdge* edge = NULL;
	if(nbEdges < MAX_CSDAG_EDGES){
		edge = &edges[nbEdges];
		edge->setBase(this);
		edge->setSource(source);
		edge->setProduction(production);
		edge->setSink(sink);
		edge->setConsumption(consumption);
		edge->setDelay(delay);
		nbEdges++;
	}
	else{
		// Adding an edge while the graph is already full
		exitWithCode(1001);
	}
	return edge;
}


/**
 Adding a configuration port to the graph

 @param vertex: pointer to the vertex connected to the port.
 	 	param:	pointer to the parameter connected to the port.
 	 	dir:	port direction. 0:input, 1:output.

 @return the new configuration port.
*/
PiCSDFConfigPort* PiCSDFGraph::addConfigPort(CSDAGVertex* vertex, PiCSDFParameter* param, int dir){
	PiCSDFConfigPort* configPort = NULL;
	if(nbConfigPorts < MAX_PISDF_CONFIG_PORTS){
		configPort = &configPorts[nbConfigPorts];
		configPort->vertex = vertex;
		configPort->parameter = param;
		configPort->direction = dir;
		nbConfigPorts++;
	}
	else{
		//TODO handle the error.
//		exitWithCode(1000);
	}
	return configPort;
}



/**
 Adding a parameter to the graph

 @param expression: //expression defining the parameter's value.

 @return the new parameter.
*/
PiCSDFParameter* PiCSDFGraph::addParameter(const char* expression){
	PiCSDFParameter* parameter = NULL;
	if(nbParameters < MAX_PISDF_CONFIG_PORTS){
		parameter = &parameters[nbParameters];
		// Parsing the expression
		globalParser.parse(expression, parameter->expression);
		nbParameters++;
	}
	else{
		//TODO handle the error.
//		exitWithCode(1000);
	}
	return parameter;
}
