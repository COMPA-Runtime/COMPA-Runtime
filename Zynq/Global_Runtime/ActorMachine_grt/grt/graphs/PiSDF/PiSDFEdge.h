
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

#ifndef PISDFEDGE_H_
#define PISDFEDGE_H_

#include <types.h>
#include "../Base/BaseEdge.h"
#include "../../expressionParser/XParser.h"
#include <expressionParser/ReversePolishNotationGenerator.h>

class BaseVertex;

class PiSDFEdge: public BaseEdge {
	UINT32 id;
	BaseVertex *source;
	BaseVertex *sink;

	// Expression defining the token production, consumption and delay (in abstract_syntax_elt)
	abstract_syntax_elt production[REVERSE_POLISH_STACK_MAX_ELEMENTS+1];
	abstract_syntax_elt consumption[REVERSE_POLISH_STACK_MAX_ELEMENTS+1];
	abstract_syntax_elt delay[REVERSE_POLISH_STACK_MAX_ELEMENTS+1];

	// Production, consumption and delay after pattern resolution.
	UINT32 productionInt;
	UINT32 consumptionInt;
	UINT32 delayInt;

	UINT32 tempId; // Used while creating a topology matrix.
public:
	PiSDFEdge() {
		consumptionInt = productionInt = delayInt = 0;
	}


	// Auto-generated setters and getters.
    UINT32 getId() const
    {
        return id;
    }

    void setId(UINT32 id)
    {
        this->id = id;
    }

    abstract_syntax_elt* getConsumption()
    {
        return consumption;
    }

    UINT32 getConsumptionInt() const
    {
        return consumptionInt;
    }

    abstract_syntax_elt* getDelay()
    {
        return delay;
    }

    UINT32 getDelayInt() const
    {
        return delayInt;
    }

    abstract_syntax_elt* getProduction()
    {
        return production;
    }

    UINT32 getProductionInt() const
    {
        return productionInt;
    }

    BaseVertex *getSink() const
    {
        return sink;
    }

    BaseVertex *getSource() const
    {
        return source;
    }

    UINT32 getTempId() const
    {
        return tempId;
    }





    void setConsumption(const char* consumption)
    {
    	globalParser.parse(consumption, this->consumption);
    }

    void setConsumtionInt(UINT32 consumtionInt)
    {
        this->consumptionInt = consumtionInt;
    }

    void setDelay(const char* delay)
    {
    	globalParser.parse(delay, this->delay);
    }

    void setDelayInt(UINT32 delayInt)
    {
        this->delayInt = delayInt;
    }

    void setProduction(const char* production)
    {
    	globalParser.parse(production, this->production);
    }

    void setProductionInt(UINT32 productionInt)
    {
        this->productionInt = productionInt;
    }

    void setSink(BaseVertex *sink)
    {
        this->sink = sink;
    }

    void setSource(BaseVertex *source)
    {
        this->source = source;
    }

    void setTempId(UINT32 tempId)
    {
        this->tempId = tempId;
    }

};

#endif /* PISDFEDGE_H_ */