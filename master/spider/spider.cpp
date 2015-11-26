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

#include "spider.h"

#include <stdio.h>
#include <launcher/Launcher.h>

Spider* Spider::spider_ = 0;

Spider::Spider(SpiderConfig cfg){
	if(spider_)
		throw "Try to create 2 Spider instances";

	spider_ = this;

	setMemAllocType(cfg.memAllocType, (long)cfg.memAllocStart, cfg.memAllocSize);
	setSchedulerType(cfg.schedulerType);
	setSrdagStack(cfg.srdagStack);
	setTransfoStack(cfg.transfoStack);
	setActorPrecedence(cfg.useActorPrecedence);
	setGraphOptim(cfg.useGraphOptim);
}

Spider::~Spider(){
	if(srdag_ != 0)
		delete srdag_;
	if(memAlloc_ != 0)
		delete memAlloc_;
	if(scheduler_ != 0)
		delete scheduler_;
	if(srdagStack_ != 0)
		delete srdagStack_;
	if(transfoStack_ != 0)
		delete transfoStack_;
}

Spider* Spider::get(){
	return spider_;
}

void Spider::idle(){
	for(int lrt=0; lrt<archi_->getNPE(); lrt++){
		if(lrt != archi_->getSpiderPeIx()){
			archi_->desactivatePE(lrt);
		}
	}
}

void Spider::iterate(){
	delete srdag_;
	srdagStack_->freeAll();
	memAlloc_->reset();

	srdag_ = new SRDAGGraph(srdagStack_);

	jit_ms(pisdf_, archi_, srdag_,
			transfoStack_, memAlloc_, scheduler_,
			useGraphOptim_,
			useActorPrecedence_);
}

void Spider::setGraphOptim(bool useGraphOptim){
	useGraphOptim_ = useGraphOptim;
}

void Spider::setActorPrecedence(bool useActorPrecedence){
	useActorPrecedence_ = useActorPrecedence;
}

void Spider::setArchi(Archi* archi){
	archi_ = archi;
}

void Spider::setGraph(PiSDFGraph* graph){
	pisdf_ = graph;
}

void Spider::setMemAllocType(MemAllocType type, int start, int size){
	if(memAlloc_ != 0){
		delete memAlloc_;
	}
	switch(type){
	case MEMALLOC_DUMMY:
		memAlloc_ = new DummyMemAlloc(start, size);
		break;
	case MEMALLOC_SPECIAL_ACTOR:
		memAlloc_ = new SpecialActorMemAlloc(start, size);
		break;
	}
}

void Spider::setSchedulerType(SchedulerType type){
	if(scheduler_ != 0){
		delete scheduler_;
	}
	switch(type){
	case SCHEDULER_LIST:
		scheduler_ = new ListScheduler();
		break;
	}
}

void Spider::setSrdagStack(StackConfig cfg){
	if(srdagStack_ != 0){
		delete srdagStack_;
	}
	switch(cfg.type){
	case STACK_DYNAMIC:
		srdagStack_ = new DynStack(cfg.name);
		break;
	case STACK_STATIC:
		srdagStack_ = new StaticStack(cfg.name, cfg.start, cfg.size);
		break;
	}
}

void Spider::setTransfoStack(StackConfig cfg){
	if(transfoStack_ != 0){
		delete transfoStack_;
	}
	switch(cfg.type){
	case STACK_DYNAMIC:
		transfoStack_ = new DynStack(cfg.name);
		break;
	case STACK_STATIC:
		transfoStack_ = new StaticStack(cfg.name, cfg.start, cfg.size);
		break;
	}
}

SRDAGGraph* Spider::getLastSRDAG(){
	return srdag_;
}

static Time start = 0;

void Spider::startMonitoring(){
	if(start != 0)
		throw "Try to monitor 2 different things in the same time";
	start = Platform::get()->getTime();
}

void Spider::endMonitoring(TraceSpiderType type){
	if(start == 0)
		throw "End monitor with no starting point";
	Launcher::get()->sendTraceSpider(type, start, Platform::get()->getTime());
	start = 0;
}

static char* regenerateColor(int refInd){
	static char color[8];
	color[0] = '\0';

	int ired = (refInd & 0x3)*50 + 100;
	int igreen = ((refInd >> 2) & 0x3)*50 + 100;
	int iblue = ((refInd >> 4) & 0x3)*50 + 100;
	char red[5];
	char green[5];
	char blue[5];
	if(ired <= 0xf){
		sprintf(red,"0%x",ired);
	}
	else{
		sprintf(red,"%x",ired);
	}

	if(igreen <= 0xf){
		sprintf(green,"0%x",igreen);
	}
	else{
		sprintf(green,"%x",igreen);
	}

	if(iblue <= 0xf){
		sprintf(blue,"0%x",iblue);
	}
	else{
		sprintf(blue,"%x",iblue);
	}

	strcpy(color,"#");
	strcat(color,red);
	strcat(color,green);
	strcat(color,blue);

	return color;
}

static inline void printGrantt_SRDAGVertex(int ganttFile, int latexFile, Archi* archi, SRDAGVertex* vertex, Time start, Time end, int lrtIx, float latexScaling){
	char name[100];
	static int i=0;
	vertex->toString(name, 100);

	Platform::get()->fprintf(ganttFile, "\t<event\n");
	Platform::get()->fprintf(ganttFile, "\t\tstart=\"%u\"\n", 	start);
	Platform::get()->fprintf(ganttFile, "\t\tend=\"%u\"\n",		end);
	Platform::get()->fprintf(ganttFile, "\t\ttitle=\"%s_%d_%d\"\n", 	name, vertex->getIterId(), vertex->getRefId());
	Platform::get()->fprintf(ganttFile, "\t\tmapping=\"%s\"\n", 	archi->getPEName(lrtIx));
	Platform::get()->fprintf(ganttFile, "\t\tcolor=\"%s\"\n", regenerateColor(i++));
	Platform::get()->fprintf(ganttFile, "\t\t>Step_%d.</event>\n", name);

	/* Latex File */
	Platform::get()->fprintf(latexFile, "%f,", start/latexScaling); /* Start */
	Platform::get()->fprintf(latexFile, "%f,", end/latexScaling); /* Duration */
	Platform::get()->fprintf(latexFile, "%d,", lrtIx); /* Core index */

	if(vertex->getFctId() == 7){
		Platform::get()->fprintf(latexFile, "color%d\n", vertex->getIterId()); /* Color */
	}else Platform::get()->fprintf(latexFile, "c\n"); /* Color */
//	if(vertex->getFctId() != -1)
//		Platform::get()->fprintf(latexFile, "color%d\n", vertex->getFctId()); /* Color */
//	else
//		Platform::get()->fprintf(latexFile, "color%d\n", 15); /* Color */

}

static const char* spiderTaskName[9] = {
		"",
		"Graph handling",
		"Memory Allocation",
		"Task Scheduling",
		"Graph Optimization",
		"Tmp 0",
		"Tmp 1",
		"Tmp 2",
		"Tmp 3"
};

void Spider::printGantt(Archi* archi, SRDAGGraph* srdag, const char* ganttPath, const char* latexPath, ExecutionStat* stat){
	int ganttFile = Platform::get()->fopen(ganttPath);
	int latexFile = Platform::get()->fopen(latexPath);

	float latexScaling = 1000;

	// Writing header
	Platform::get()->fprintf(ganttFile, "<data>\n");
	Platform::get()->fprintf(latexFile, "start,end,core,color\n");

	// Popping data from Trace queue.
	stat->mappingTime = 0;
	stat->graphTime = 0;
	stat->optimTime = 0;
	stat->schedTime = 0;
	stat->globalEndTime = 0;

	stat->forkTime = 0;
	stat->joinTime = 0;
	stat->rbTime = 0;
	stat->brTime = 0;
	stat->nExecSRDAGActor = 0;
	stat->nSRDAGActor = srdag->getNVertex();
	stat->nSRDAGEdge = srdag->getNEdge();
	stat->nPiSDFActor = 0;

	stat->memoryUsed = memAlloc_->getMemUsed();

	TraceMsg* traceMsg;
	int n = Launcher::get()->getNLaunched();
	while(n){
		if(Platform::getSpiderCommunicator()->trace_start_recv((void**)&traceMsg)){
			switch (traceMsg->msgIx) {
				case TRACE_JOB:{
					SRDAGVertex* vertex = srdag->getVertexFromIx(traceMsg->srdagIx);
					Time execTime = traceMsg->end - traceMsg->start;

					static int baseTime=0;
//					if(strcmp(vertex->getReference()->getName(),"src") == 0){
//						baseTime = traceMsg->start;
//					}

					printGrantt_SRDAGVertex(
							ganttFile,
							latexFile,
							archi,
							vertex,
							traceMsg->start-baseTime,
							traceMsg->end-baseTime,
							traceMsg->lrtIx,
							latexScaling);

					/* Update Stats */
					stat->globalEndTime = std::max(traceMsg->end-baseTime, stat->globalEndTime);
					stat->nExecSRDAGActor++;

					switch(vertex->getType()){
						case SRDAG_NORMAL:{
							int i;
							int lrtType = archi->getPEType(traceMsg->lrtIx);
							for(i=0; i<stat->nPiSDFActor; i++){
								if(stat->actors[i] == vertex->getReference()){
									stat->actorTimes[i][lrtType] += execTime;
									stat->actorIterations[i][lrtType]++;

									stat->actorFisrt[i] = std::min(stat->actorFisrt[i], traceMsg->start);
									stat->actorLast[i] = std::max(stat->actorLast[i], traceMsg->end);
									break;
								}
							}
							if(i == stat->nPiSDFActor){
								stat->actors[stat->nPiSDFActor] = vertex->getReference();

								memset(stat->actorTimes[stat->nPiSDFActor], 0, MAX_STATS_PE_TYPES*sizeof(Time));
								memset(stat->actorIterations[stat->nPiSDFActor], 0, MAX_STATS_PE_TYPES*sizeof(Time));

								stat->actorTimes[stat->nPiSDFActor][lrtType] += execTime;
								stat->actorIterations[i][lrtType]++;
								stat->nPiSDFActor++;

								stat->actorFisrt[i] = traceMsg->start;
								stat->actorLast[i] = traceMsg->end;
							}
							break;}
						case SRDAG_BROADCAST:
							stat->brTime += execTime;
							break;
						case SRDAG_FORK:
							stat->forkTime += execTime;
							break;
						case SRDAG_JOIN:
							stat->joinTime += execTime;
							break;
						case SRDAG_ROUNDBUFFER:
							stat->rbTime += execTime;
							break;
						case SRDAG_INIT:
						case SRDAG_END:
							break;
					}

					break;}
				case TRACE_SPIDER:{
//					break;

					static int i=0;
					/* Gantt File */
					Platform::get()->fprintf(ganttFile, "\t<event\n");
					Platform::get()->fprintf(ganttFile, "\t\tstart=\"%u\"\n", 	traceMsg->start);
					Platform::get()->fprintf(ganttFile, "\t\tend=\"%u\"\n",		traceMsg->end);
					Platform::get()->fprintf(ganttFile, "\t\ttitle=\"%s\"\n", 	spiderTaskName[traceMsg->spiderTask]);
					Platform::get()->fprintf(ganttFile, "\t\tmapping=\"%s\"\n", archi->getPEName(traceMsg->lrtIx));
					Platform::get()->fprintf(ganttFile, "\t\tcolor=\"%s\"\n", 	regenerateColor(i++));
					Platform::get()->fprintf(ganttFile, "\t\t>Step_%d.</event>\n", spiderTaskName[traceMsg->spiderTask]);

					stat->schedTime = std::max(traceMsg->end, stat->schedTime);

					switch(traceMsg->spiderTask){
					case TRACE_SPIDER_GRAPH:
						stat->graphTime += traceMsg->end - traceMsg->start;
						break;
					case TRACE_SPIDER_ALLOC:
						throw "Unhandle trace";
						break;
					case TRACE_SPIDER_SCHED:
						stat->mappingTime += traceMsg->end - traceMsg->start;
						break;
					case TRACE_SPIDER_OPTIM:
						stat->optimTime += traceMsg->end - traceMsg->start;
						break;
					}

					/* Latex File */
					Platform::get()->fprintf(latexFile, "%f,", traceMsg->start/latexScaling); /* Start */
					Platform::get()->fprintf(latexFile, "%f,", traceMsg->end/latexScaling); /* Duration */
					Platform::get()->fprintf(latexFile, "%d,", 0); /* Core index */
					Platform::get()->fprintf(latexFile, "colorSched\n",15); /* Color */
					break;}
				default:
					throw "Unhandled trace msg";
					break;
			}
			Platform::getSpiderCommunicator()->trace_end_recv();
			n--;
		}
	}
	Launcher::get()->rstNLaunched();

	Platform::get()->fprintf(ganttFile, "</data>\n");
	Platform::get()->fclose(ganttFile);

	Platform::get()->fclose(latexFile);

	stat->execTime = stat->globalEndTime - stat->schedTime;
}
