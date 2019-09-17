/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018 - 2019)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Daniel Madroñal <daniel.madronal@upm.es> (2019)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018 - 2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2018)
 * rlazcano <raquel.lazcano@upm.es> (2019)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#include <cinttypes>
#include "spider.h"

#include <graphs/PiSDF/PiSDFCommon.h>
#include <graphs/SRDAG/SRDAGCommon.h>
#include <graphs/SRDAG/SRDAGGraph.h>
#include <graphs/PiSDF/PiSDFGraph.h>
#include <graphs/PiSDF/PiSDFEdge.h>

#include <scheduling/MemAlloc/SpecialActorMemAlloc.h>
#include <scheduling/MemAlloc/DummyMemAlloc.h>
#include <scheduling/Scheduler.h>
#include <scheduling/MemAlloc.h>

#include <scheduling/Scheduler/ListSchedulerOnTheGo.h>
#include <scheduling/Scheduler/RoundRobinScattered.h>
#include <scheduling/Scheduler/ListScheduler.h>
#include <scheduling/Scheduler/RoundRobin.h>

#include <graphTransfo/GraphTransfo.h>

#include <SpiderCommunicator.h>

#include <monitor/TimeMonitor.h>
#include <launcher/Launcher.h>
#include <Logger.h>
#include <scheduling/MemAlloc/DummyPiSDFMemAlloc.h>
#include <scheduling/Scheduler/GreedyScheduler.h>
#include <graphs/Archi/Archi.h>

#include "platformPThread.h"
#include <limits>
#include <math.h>

// #ifndef __k1__
// #include <HAL/hal/hal_ext.h>
// #endif

#ifdef __k1__
#define CHIP_FREQ ((float)(__bsp_frequency)/(1000*1000))
#endif

#ifndef CHIP_FREQ
#define CHIP_FREQ (1)
#endif

static Archi *archi_ = nullptr;
static PiSDFGraph *pisdf_ = nullptr;
static SRDAGGraph *srdag_ = nullptr;

static MemAlloc *memAlloc_ = nullptr;
static Scheduler *scheduler_ = nullptr;
//static PlatformMPPA* platform_;
static PlatformPThread *platform_ = nullptr;
static SRDAGSchedule *schedule_ = nullptr;

static bool verbose_;
static bool useGraphOptim_;
static bool useActorPrecedence_;
static bool traceEnabled_;
static bool papifyFeedbackEnabled_;
static bool apolloEnabled_;
static bool apolloCompiled_;

// energy awareness info
static bool energyAwareness_;
static double performanceObjective_;
static Time startingExecutionTime_;
static Time endingExecutionTime_;

static std::map<std::uint32_t, std::vector<std::uint32_t>> peIdPerPeType_; 
static std::map<std::uint32_t, std::uint32_t> pesBeingDisabled_; 

static std::map<const char*, Param> dynamicParameters_;
static std::uint32_t numDynamicParameters_;
static std::vector<std::map<std::uint32_t, std::uint32_t>> configsAlreadyUsed_; 
static std::map<std::uint32_t, std::uint32_t> pesBestConfig_; 
static double bestEnergy_;
static double bestObjective_;
static bool energyAlreadyOptimized_;
static bool solutionShown_;
static bool aboveBelowObjective_; // 1 current config is above objective -- 0 current config is below objective
static std::uint32_t timesObjectiveMissed_; // Fail safe when not reaching the objective
static bool forcedUpdate_; // True when the performance of the best config does not reach the objective 10 times in a row

static std::map<std::map<const char*, Param>, std::vector<std::map<std::uint32_t, std::uint32_t>>> configsAlreadyUsedBackup_; 
static std::map<std::map<const char*, Param>, std::map<std::uint32_t, std::uint32_t>> pesBestConfigBackup_; 
static std::map<std::map<const char*, Param>, std::map<std::uint32_t, std::uint32_t>> pesBeingDisabledBackup_; 
static std::map<std::map<const char*, Param>, double> bestEnergyBackup_; 
static std::map<std::map<const char*, Param>, double> bestObjectiveBackup_; 
static std::map<std::map<const char*, Param>, bool> energyAlreadyOptimizedBackup_; 
static std::map<std::map<const char*, Param>, bool> aboveBelowObjectiveBackup_; 

static bool containsDynamicParam(PiSDFGraph *const graph) {
    for (int i = 0; i < graph->getNParam(); ++i) {
        auto *param = graph->getParam(i);
        if (param->isDynamic()) {
            numDynamicParameters_ = numDynamicParameters_ + 1;
            return true;
        }
    }
    return false;
}

static bool isGraphStatic(PiSDFGraph *const graph) {
    bool isStatic = !containsDynamicParam(graph);
    for (int j = 0; j < graph->getNBody(); ++j) {
        PiSDFVertex *vertex = graph->getBody(j);
        if (vertex->isHierarchical()) {
            auto *subGraph = vertex->getSubGraph();
            subGraph->setGraphStaticProperty(isGraphStatic(subGraph));
            isStatic &= subGraph->isGraphStatic();
        }
    }
    return isStatic;
}

void Spider::initStacks(SpiderStackConfig &cfg) {
    StackMonitor::initStack(ARCHI_STACK, cfg.archiStack);
    StackMonitor::initStack(PISDF_STACK, cfg.pisdfStack);
    StackMonitor::initStack(SRDAG_STACK, cfg.srdagStack);
    StackMonitor::initStack(TRANSFO_STACK, cfg.transfoStack);
}

static int counter;
void Spider::init(SpiderConfig &cfg, SpiderStackConfig &stackConfig) {
    Logger::initializeLogger();
    counter = 0;
    setGraphOptim(cfg.useGraphOptim);

    setMemAllocType(cfg.memAllocType, cfg.memAllocStart, cfg.memAllocSize);
    setSchedulerType(cfg.schedulerType);

    setVerbose(cfg.verbose);
    setTraceEnabled(cfg.traceEnabled);
    setApolloEnabled(cfg.apolloEnabled);
    setApolloCompiled(cfg.apolloCompiled);

    //TODO: add a switch between the different platform
    platform_ = new PlatformPThread(cfg, stackConfig);

    setPapifyFeedbackEnabled(cfg.feedbackPapifyInfo);

    setEnergyAwareness(cfg.energyAwareness);
    if(energyAwareness_){
        setPerformanceObjective(cfg.performanceObjective);
        Spider::setUpEnergyAwareness();
        energyAlreadyOptimized_ = false;
        solutionShown_ = false;
        bestEnergy_ = std::numeric_limits<double>::max();
        bestObjective_ = 0.0;
        numDynamicParameters_ = 0;
        timesObjectiveMissed_ = 0;
        forcedUpdate_ = false;
    }

    if (traceEnabled_) {
        Launcher::get()->sendEnableTrace(-1);
    }
    #ifdef APOLLO_AVAILABLE
        if (apolloEnabled_ && apolloCompiled_) {
            initApolloForDataflow();
        }else if(apolloCompiled_){
            disableApollo();
        }
    #endif
//    Logger::enable(LOG_JOB);
}

void Spider::iterate() {
    Platform::get()->rstTime();

    // The behavior when using energy awareness is slightly different
    if (pisdf_->isGraphStatic()) {
        if(energyAwareness_){
            if(energyAlreadyOptimized_){
                if(!srdag_){
                    energyAwarenessApplyConfig();
                    /* On final iteration, the schedule is created */
                    srdag_ = new SRDAGGraph();
                    schedule_ = static_scheduler(srdag_, memAlloc_, scheduler_);
                }
            } else {
                delete srdag_;
                StackMonitor::freeAll(SRDAG_STACK);
                memAlloc_->reset();
                srdag_ = new SRDAGGraph();
                schedule_ = static_scheduler(srdag_, memAlloc_, scheduler_);
            }
        }else{
            if (!srdag_) {
                /* On first iteration, the schedule is created */
                srdag_ = new SRDAGGraph();
                schedule_ = static_scheduler(srdag_, memAlloc_, scheduler_);
            }
        }
        /* Run the schedule */
        Spider::setStartingTime();
        schedule_->executeAndRun();
        Spider::setEndTime();
        schedule_->restartSchedule();
    } else {
        delete srdag_;
        StackMonitor::freeAll(SRDAG_STACK);
        memAlloc_->reset();
        srdag_ = new SRDAGGraph();
        jit_ms(pisdf_, archi_, srdag_, memAlloc_, scheduler_);
    }
    
    /** Process PAPIFY feedback **/
    if(papifyFeedbackEnabled_){
        Platform::get()->processPapifyFeedback(srdag_);
    }

    /** Wait for LRTs to finish **/
    Platform::get()->rstJobIxRecv();

    /** Compute energy **/
    if(energyAwareness_){
        energyAwarenessAnalyzeExecution();
        energyAwarenessPrepareNextExecution();
    }
}

void Spider::setStartingTime() {
    startingExecutionTime_ = Platform::get()->getTime();
}

void Spider::setEndTime() {
    endingExecutionTime_ = Platform::get()->getTime();
}

unsigned long Spider::getExecutionTime() {
    return (endingExecutionTime_ - startingExecutionTime_) / CHIP_FREQ;
}

double Spider::computeEnergy(SRDAGGraph *srdag, Archi *archi, double fpsEstimation) {
    double energyIter = 0.0;
    std::vector<std::uint32_t> pesUsed;
    for (int i = 0; i < srdag->getNVertex(); i++) {
        SRDAGVertex *vertex = srdag->getVertex(i);
        if (vertex->getType() == SRDAG_NORMAL) {
            int peUsed = vertex->getScheduleJob()->getMappedPE();
            int peType = archi_->getPEFromSpiderID(peUsed)->getHardwareType();
            auto piVertex = vertex->getReference();
            double energy = piVertex->getEnergyOnPEType(peType);
            energyIter = energyIter + energy;      
        }
    }
    // we asume that energy for each actor is given in uJ
    double energyApp = (energyIter / (1000.0 * 1000.0))  * fpsEstimation;
    // as power is provided in W and we want energy per second, values are equivalent
    double energyPlatform = archi_->getBasePower();
    for (unsigned int i = 0; i < archi->getNPE(); i++) {
        auto pe = archi->getPEFromSpiderID(i);
        if (pe->isEnabled()) {
            energyPlatform = energyPlatform + pe->getPower();            
        }
    }
    return energyApp + energyPlatform;    
}

double Spider::computeFps() {
    unsigned long execTime = Spider::getExecutionTime();
    // execution time is given in ns
    return (1000.0 * 1000.0 * 1000.0) / (double) execTime;
}

void Spider::setUpEnergyAwareness() {
    auto spiderId = archi_->getSpiderGRTID();
    for (std::uint32_t i = 0; i < archi_->getNPE(); i++) {
        auto pe = archi_->getPEFromSpiderID(i);
        if (i != spiderId) {
            auto peType = pe->getHardwareType();
            auto it = peIdPerPeType_.find(peType);
            if (it != peIdPerPeType_.end()) {
                it->second.push_back(i);
            } else {
                std::vector<std::uint32_t> peIdSet;
                peIdSet.push_back(i);
                peIdPerPeType_.insert(std::make_pair(peType, peIdSet));
            }          
        }
    }
    for (auto it = peIdPerPeType_.begin(); it != peIdPerPeType_.end(); it++) {
        pesBeingDisabled_.insert(std::make_pair(it->first, 0));
        pesBestConfig_.insert(std::make_pair(it->first, 0));
    }
}

void Spider::checkExecutionPerformance(double fpsEstimation, double energyConsumed) {
    aboveBelowObjective_ = false;
    if (fpsEstimation >= performanceObjective_) {
        aboveBelowObjective_ = true;
        if (bestEnergy_ > energyConsumed || forcedUpdate_) {
            bestEnergy_ = energyConsumed;
            bestObjective_ = fpsEstimation;
            for (auto it = pesBeingDisabled_.begin(); it != pesBeingDisabled_.end(); it++) {
                pesBestConfig_[it->first] = it->second;
            }
        } 
    }else if (fpsEstimation > bestObjective_ && bestEnergy_ == std::numeric_limits<double>::max()) {
        bestObjective_ = fpsEstimation;
        for (auto it = pesBeingDisabled_.begin(); it != pesBeingDisabled_.end(); it++) {
            pesBestConfig_[it->first] = it->second;
        }
    }
    forcedUpdate_ = false;
}

bool Spider::generateNextEnergyConfiguration() {
    return generateNextFineGrainEnergyConfiguration();
}

bool Spider::generateNextFineGrainEnergyConfiguration(){
    bool alreadyUsingTheMaximum = true;
    for (auto it = pesBeingDisabled_.begin(); it != pesBeingDisabled_.end(); it++) {
        if(aboveBelowObjective_){ // As we are modifying the number of disabled PEs, we have to increase the disable PEs when we are above the number
            if(pesBeingDisabled_[it->first] == peIdPerPeType_[it->first].size()){
                pesBeingDisabled_[it->first] = 0;
            }else{
                pesBeingDisabled_[it->first] = pesBeingDisabled_[it->first] + 1; 
                break;
            }      
        } else {
            if(pesBeingDisabled_[it->first] == 0){
                pesBeingDisabled_[it->first] = peIdPerPeType_[it->first].size();
            }else{
                pesBeingDisabled_[it->first] = pesBeingDisabled_[it->first] - 1;  
                alreadyUsingTheMaximum = false;
                break;
            }      
        }
    }
    auto it = std::find(configsAlreadyUsed_.begin(), configsAlreadyUsed_.end(), pesBeingDisabled_);
    if (it == configsAlreadyUsed_.end()) {
        return true;
    } else if(!aboveBelowObjective_ && bestEnergy_ != std::numeric_limits<double>::max() && !alreadyUsingTheMaximum){
        timesObjectiveMissed_++;
        if(timesObjectiveMissed_ == 10){
            timesObjectiveMissed_ = 0;
            forcedUpdate_ = true;
            printf("Increasing number of enabled PEs because the objective has been missed 10 times\n");
            return true;
        }
    }
    return false;
}

void Spider::energyAwarenessApplyConfig(){
    auto it = std::find(configsAlreadyUsed_.begin(), configsAlreadyUsed_.end(), pesBeingDisabled_);
    if (it == configsAlreadyUsed_.end()) {
        configsAlreadyUsed_.push_back(pesBeingDisabled_);
    }
    std::map<std::uint32_t, std::uint32_t> pesAlreadyDisbled;
    for (auto it = pesBeingDisabled_.begin(); it != pesBeingDisabled_.end(); it++) {
        pesAlreadyDisbled.insert(std::make_pair(it->first, 0));
    }
    for(auto it = peIdPerPeType_.begin(); it != peIdPerPeType_.end(); it++){
        for(auto itInner = it->second.begin(); itInner != it->second.end(); itInner++){
            archi_->activatePE(archi_->getPEFromSpiderID(*itInner));
        }
    }
    for(auto it = peIdPerPeType_.begin(); it != peIdPerPeType_.end(); it++){
        for(auto itInner = it->second.begin(); itInner != it->second.end(); itInner++){
            if(pesAlreadyDisbled[it->first] == pesBeingDisabled_[it->first]){
                break;
            }
            archi_->deactivatePE(archi_->getPEFromSpiderID(*itInner));
            pesAlreadyDisbled[it->first] = pesAlreadyDisbled[it->first] + 1;
        }
    }
}

void Spider::energyAwarenessAnalyzeExecution(){
    double fpsEstimation = computeFps();
    double energyConsumed = computeEnergy(srdag_, archi_, fpsEstimation);

    checkExecutionPerformance(fpsEstimation, energyConsumed);
}

void Spider::energyAwarenessPrepareNextExecution(){
    if(!generateNextEnergyConfiguration()){
        energyAlreadyOptimized_ = true;
        pesBeingDisabled_ = pesBestConfig_;
        for (auto it = pesBestConfig_.begin(); it != pesBestConfig_.end(); it++) {
            pesBeingDisabled_[it->first] = it->second;
        }
        if(!solutionShown_){
            solutionShown_ = true;
            std::uint32_t totalCoresUsed = 0;
            for (auto it = pesBestConfig_.begin(); it != pesBestConfig_.end(); it++) {
                totalCoresUsed = totalCoresUsed + peIdPerPeType_[it->first].size() - it->second;
            }
            if(bestEnergy_ == std::numeric_limits<double>::max()){
                printf("Best FPS %f. Objective not reachable --> ", bestObjective_);    
                printf("Closer one reached using %d cores", (totalCoresUsed + 1)); // We include here the GRT
            }else{
                printf("Best FPS %f and best energy consumed = %f ---> ", bestObjective_, bestEnergy_);   
                printf("Reached using %d cores", (totalCoresUsed + 1)); // We include here the GRT             
            }
            printf("\n");
            printf("Using config (PEType: disabled/total): ");
            for (auto it = pesBestConfig_.begin(); it != pesBestConfig_.end(); it++) {
                printf("%d: %d/%lu --- ", it->first, it->second, peIdPerPeType_[it->first].size());
            }
            printf("\n");
        }
    }
}

bool Spider::getEnergyAwareness() {
    return energyAwareness_;
}

void Spider::recoverEnergyAwarenessOrDefault() {
    timesObjectiveMissed_ = 0;
    // All the energy-awareness related variables are stored at the same time
    // So we only check one and we consider that everything is properly done
    auto itCheckerEnergyAlreadyOptimized = energyAlreadyOptimizedBackup_.find(dynamicParameters_);
    if(itCheckerEnergyAlreadyOptimized != energyAlreadyOptimizedBackup_.end()){
        energyAlreadyOptimized_ = energyAlreadyOptimizedBackup_[dynamicParameters_];
        aboveBelowObjective_ = aboveBelowObjectiveBackup_[dynamicParameters_];
        pesBestConfig_ = pesBestConfigBackup_[dynamicParameters_];
        pesBeingDisabled_ = pesBeingDisabledBackup_[dynamicParameters_]; 
        bestEnergy_ = bestEnergyBackup_[dynamicParameters_];
        bestObjective_ = bestObjectiveBackup_[dynamicParameters_];
        configsAlreadyUsed_ = configsAlreadyUsedBackup_[dynamicParameters_];
    }else{
        energyAlreadyOptimized_ = false;        
        for (auto it = pesBeingDisabled_.begin(); it != pesBeingDisabled_.end(); it++) {
            pesBestConfig_[it->first] = it->second;
        }
        bestEnergy_ = std::numeric_limits<double>::max();
        bestObjective_ = 0.0;
        configsAlreadyUsed_.clear();
    }
    solutionShown_ = false;
}

void Spider::setNewDynamicParamsEnergyAwareness(std::map<const char*, Param> dynamicParamsMap) {
    //Store current best ones
    configsAlreadyUsedBackup_[dynamicParameters_] = configsAlreadyUsed_; 
    pesBestConfigBackup_[dynamicParameters_] = pesBestConfig_; 
    pesBeingDisabledBackup_[dynamicParameters_] = pesBeingDisabled_; 
    bestEnergyBackup_[dynamicParameters_] = bestEnergy_; 
    bestObjectiveBackup_[dynamicParameters_] = bestObjective_; 
    energyAlreadyOptimizedBackup_[dynamicParameters_] = energyAlreadyOptimized_;
    aboveBelowObjectiveBackup_[dynamicParameters_] = aboveBelowObjective_;

    //Check the need to update the config
    bool dirtyEnergyConfig = false;
    for (auto it = dynamicParamsMap.begin(); it != dynamicParamsMap.end(); it++) {
        auto itChecker = dynamicParameters_.find(it->first);
        if(itChecker == dynamicParameters_.end()){
            dynamicParameters_.insert(std::make_pair(it->first, it->second));
            dirtyEnergyConfig = true;
        } else {
            if(dynamicParameters_[it->first] != it->second){
                dirtyEnergyConfig = true;
                dynamicParameters_[it->first] = it->second;
            }
        }        
    }
    //Update config if needed
    if(dirtyEnergyConfig){
        recoverEnergyAwarenessOrDefault();
    }
}

static int getReservedMemoryForGraph(PiSDFGraph *graph, int currentMemReserved) {
    auto *job = CREATE(TRANSFO_STACK, transfoJob);
    memset(job, 0, sizeof(transfoJob));
    job->graph = graph;
    job->paramValues = CREATE_MUL(TRANSFO_STACK, job->graph->getNParam(), Param);
    for (int paramIx = 0; paramIx < job->graph->getNParam(); paramIx++) {
        PiSDFParam *param = job->graph->getParam(paramIx);
        job->paramValues[paramIx] = param->getValue();
    }
    int memReserved = currentMemReserved;
    // Compute the total memory allocation needed for delays in current graph
    for (int i = 0; i < graph->getNEdge(); i++) {
        PiSDFEdge *edge = graph->getEdge(i);
        auto nbDelays = edge->resolveDelay();
        if (nbDelays > 0 && edge->isDelayPersistent()) {
            // Compute memory offset
            int memAllocAddr = memAlloc_->getMemUsed();
            edge->setMemoryDelayAlloc(memAllocAddr);
            // Get reserved aligned size
            memReserved += memAlloc_->getReservedAlloc(nbDelays);
        }
    }
    StackMonitor::free(TRANSFO_STACK, job->paramValues);
    StackMonitor::free(TRANSFO_STACK, job);
    // Compute the total memory allocation needed for delays in subgraph
    for (int j = 0; j < graph->getNBody(); ++j) {
        PiSDFVertex *vertex = graph->getBody(j);
        if (vertex->isHierarchical()) {
            memReserved += getReservedMemoryForGraph(vertex->getSubGraph(), currentMemReserved);
        }
    }
    return memReserved;
}

void Spider::initReservedMemory() {
    PiSDFVertex *root = pisdf_->getBody(0);
    PiSDFGraph *graph = root->getSubGraph();
    // Compute the needed reserved memory for delays
    memAlloc_->reset();
    int memReserved = memAlloc_->getReservedAlloc(1);
    // Recursively go through the hierarchy
    memReserved += getReservedMemoryForGraph(graph, 0);
    fprintf(stderr, "INFO: Reserved ");
    if (memReserved < 1024) {
        fprintf(stderr, "%5.1f B", memReserved * 1.);
    } else if (memReserved < 1024 * 1024) {
        fprintf(stderr, "%5.1f KB", memReserved / 1024.);
    } else if (memReserved < 1024 * 1024 * 1024) {
        fprintf(stderr, "%5.1f MB", memReserved / (1024. * 1024.));
    } else {
        fprintf(stderr, "%5.1f GB", memReserved / (1024. * 1024. * 1024.));
    }
    fprintf(stderr, "(%#x) / ", memReserved);
    memAlloc_->printMemAllocSizeFormatted();
    fprintf(stderr, " of the shared memory\n");
    fprintf(stderr, "for delays.\n");

    memAlloc_->setReservedSize(memReserved);
    memAlloc_->reset();
}

void Spider::clean() {
    if (schedule_) {
//        schedule_->~PiSDFSchedule();
        schedule_->~SRDAGSchedule();
        StackMonitor::free(TRANSFO_STACK, schedule_);
    }

    if (archi_) {
        archi_->~Archi();
        StackMonitor::free(ARCHI_STACK, archi_);
    }

    if (pisdf_) {
        pisdf_->~PiSDFGraph();
        StackMonitor::free(PISDF_STACK, pisdf_);
        StackMonitor::freeAll(PISDF_STACK);
    }

    delete srdag_;
    delete memAlloc_;
    delete scheduler_;
    delete platform_;

    /* === Checking stacks state === */

    StackMonitor::freeAll(ARCHI_STACK);
    StackMonitor::freeAll(TRANSFO_STACK);
    StackMonitor::freeAll(SRDAG_STACK);
    StackMonitor::freeAll(PISDF_STACK);

    /* === Cleaning the Stacks === */

    StackMonitor::clean(ARCHI_STACK);
    StackMonitor::clean(TRANSFO_STACK);
    StackMonitor::clean(SRDAG_STACK);
    StackMonitor::clean(PISDF_STACK);
}


void Spider::setGraphOptim(bool useGraphOptim) {
    useGraphOptim_ = useGraphOptim;
}

void Spider::setVerbose(bool verbose) {
    verbose_ = verbose;
}

void Spider::setActorPrecedence(bool useActorPrecedence) {
    useActorPrecedence_ = useActorPrecedence;
}

void Spider::setTraceEnabled(bool traceEnabled) {
    traceEnabled_ = traceEnabled;
}

void Spider::setPapifyFeedbackEnabled(bool papifyFeedbackEnabled) {
    papifyFeedbackEnabled_ = papifyFeedbackEnabled;
}

void Spider::setEnergyAwareness(bool energyAwareness) {
    energyAwareness_ = energyAwareness;
}

void Spider::setPerformanceObjective(double performanceObjective) {
    performanceObjective_ = performanceObjective;
}

void Spider::setApolloEnabled(bool apolloEnabled) {
    apolloEnabled_ = apolloEnabled;
}

void Spider::setApolloCompiled(bool apolloCompiled){
    apolloCompiled_ = apolloCompiled;
}

bool Spider::getVerbose() {
    return verbose_;
}

bool Spider::getGraphOptim() {
    return useGraphOptim_;
}

bool Spider::getActorPrecedence() {
    return useActorPrecedence_;
}

bool Spider::getTraceEnabled() {
    return traceEnabled_;
}

bool Spider::getApolloEnabled() {
    return apolloEnabled_;
}

bool Spider::getApolloCompiled() {
    return apolloCompiled_;
}

void Spider::setArchi(Archi *archi) {
    archi_ = archi;
}

void Spider::setGraph(PiSDFGraph *graph) {
    pisdf_ = graph;

    // Detect the static property of the graph
    pisdf_->setGraphStaticProperty(isGraphStatic(pisdf_->getBody(0)->getSubGraph()));

    if (pisdf_->isGraphStatic()) {
        Platform::get()->fprintf(stderr, "Graph [%s] is static.\n", pisdf_->getBody(0)->getName());
    } else {
        Platform::get()->fprintf(stderr, "Graph [%s] is not fully static.\n", pisdf_->getBody(0)->getName());
    }
}

PiSDFGraph *Spider::getGraph() {
    return pisdf_;
}

Archi *Spider::getArchi() {
    return archi_;
}

void Spider::setMemAllocType(MemAllocType type, int start, int size) {
    /** If a memAlloc_ already existed, we delete it**/
    delete memAlloc_;
    switch (type) {
        case MEMALLOC_DUMMY:
            memAlloc_ = new DummyMemAlloc(start, size);
            break;
        case MEMALLOC_SPECIAL_ACTOR:
            memAlloc_ = new SpecialActorMemAlloc(start, size);
            break;
        default:
            throwSpiderException("Unsupported type of Memory Allocation.\n");
    }
}

void Spider::setSchedulerType(SchedulerType type) {
    /** If a scheduler_ already existed, we delete it**/
    delete scheduler_;
    switch (type) {
        case SCHEDULER_LIST:
            scheduler_ = new ListScheduler();
            break;
        case SCHEDULER_GREEDY:
            scheduler_ = new GreedyScheduler();
            break;
        case SCHEDULER_LIST_ON_THE_GO:
            scheduler_ = new ListSchedulerOnTheGo();
            break;
        case SCHEDULER_ROUND_ROBIN:
            scheduler_ = new RoundRobin();
            break;
        case SCHEDULER_ROUND_ROBIN_SCATTERED:
            scheduler_ = new RoundRobinScattered();
            break;
    }
}

void Spider::printSRDAG(const char *srdagPath) {
    return srdag_->print(srdagPath);
}

void Spider::printPiSDF(const char *pisdfPath) {
    pisdf_->getBody(0)->getSubGraph()->print(pisdfPath);
}

void Spider::printActorsStat(ExecutionStat *stat) {
    Platform::get()->fprintf(stdout, "\t%15s:\n", "Actors");
    for (int j = 0; j < stat->nPiSDFActor; j++) {
        Platform::get()->fprintf(stdout, "\t%15s:", stat->actors[j]->getName());
        for (std::uint32_t k = 0; k < archi_->getNPEType(); k++) {
            if (stat->actorIterations[j][k]) {
                Platform::get()->fprintf(stdout, "\t%lld (x%lld)",
                                         stat->actorTimes[j][k] / stat->actorIterations[j][k],
                                         stat->actorIterations[j][k]);
            } else {
                Platform::get()->fprintf(stdout, "\t%d (x%d)", 0, 0);
            }
        }
        Platform::get()->fprintf(stdout, "\n");
    }
}

static char *regenerateColor(int refInd) {
    static char color[8];
    color[0] = '\0';

    int ired = (refInd & 0x3) * 50 + 100;
    int igreen = ((refInd >> 2) & 0x3) * 50 + 100;
    int iblue = ((refInd >> 4) & 0x3) * 50 + 100;
    char red[5];
    char green[5];
    char blue[5];
    if (ired <= 0xf) {
        sprintf(red, "0%x", ired);
    } else {
        sprintf(red, "%x", ired);
    }

    if (igreen <= 0xf) {
        sprintf(green, "0%x", igreen);
    } else {
        sprintf(green, "%x", igreen);
    }

    if (iblue <= 0xf) {
        sprintf(blue, "0%x", iblue);
    } else {
        sprintf(blue, "%x", iblue);
    }

    strcpy(color, "#");
    strcat(color, red);
    strcat(color, green);
    strcat(color, blue);

    return color;
}

static void printGantt_SRDAGVertex(FILE *ganttFile, FILE *latexFile, Archi *archi, SRDAGVertex *vertex,
                                   Time start, Time end, int lrtIx, float latexScaling) {
    static char name[200];
    static int i = 0;
    vertex->toString(name, 100);

    auto *temp_str = (char *) malloc(300 * sizeof(char));


    sprintf(temp_str,
            "\t<event\n"
            "\t\tstart=\"%" PRIu64"\"\n"
            "\t\tend=\"%" PRIu64"\"\n"
            "\t\ttitle=\"%s_%d_%d\"\n"
            "\t\tmapping=\"%s\"\n"
            "\t\tcolor=\"%s\"\n"
            "\t\t>Step_%d.</event>\n",
            start,
            end,
            name, vertex->getIterId(), vertex->getRefId(),
            archi->getPEFromSpiderID(lrtIx)->getName().c_str(),
            regenerateColor(i++),
            lrtIx);

    Platform::get()->fprintf(ganttFile, "%s", temp_str);

    sprintf(temp_str,
            "%f,"
            "%f,"
            "%d,",
            start / latexScaling,
            end / latexScaling,
            lrtIx);

    if (vertex->getFctId() == 7) {
        sprintf(temp_str + strlen(temp_str), "color%d\n", vertex->getIterId());
    } else {
        sprintf(temp_str + strlen(temp_str), "c\n");
    }

    Platform::get()->fprintf(latexFile, "%s", temp_str);

    /* Latex File */
    // Platform::get()->fprintf(latexFile, "%f,", start/latexScaling); /* Start */
    // Platform::get()->fprintf(latexFile, "%f,", end/latexScaling); /* Duration */
    // Platform::get()->fprintf(latexFile, "%d,", lrtIx); /* Core index */

    // if(vertex->getFctId() == 7){
    // 	Platform::get()->fprintf(latexFile, "color%d\n", vertex->getIterId()); /* Color */
    // }else Platform::get()->fprintf(latexFile, "c\n"); /* Color */

    free(temp_str);
}

static void writeGanttForVertex(TraceMessage *message, FILE *ganttFile, FILE *latexFile, ExecutionStat *stat) {
    SRDAGVertex *vertex = srdag_->getVertexFromIx(message->getVertexID());

    auto startTimeScaled = message->getStartTime() / CHIP_FREQ;
    auto endTimeScaled = message->getEndTime() / CHIP_FREQ;

    auto execTime = message->getEllapsedTime() / CHIP_FREQ;

    Time baseTime = 0;
    // if(strcmp(vertex->getReference()->getName(),"src") == 0){
    // 	baseTime = traceMsg->start;
    // }


    printGantt_SRDAGVertex(
            ganttFile,
            latexFile,
            archi_,
            vertex,
            startTimeScaled - baseTime,
            endTimeScaled - baseTime,
            message->getLRTID(),
            1000.f);


    /* Update Stats */
    stat->globalEndTime = std::max(endTimeScaled - baseTime, stat->globalEndTime);
    stat->nExecSRDAGActor++;

    switch (vertex->getType()) {
        case SRDAG_NORMAL: {
            int i;
            auto lrtType = archi_->getPEFromSpiderID(message->getLRTID())->getHardwareType();
            auto *pisdfVertexRef = vertex->getReference();
            // Update execution time of the PiSDF actor
            auto timingOnPe = std::to_string(execTime);
            pisdfVertexRef->setTimingOnType(lrtType, timingOnPe.c_str());
            // Update global stats
            for (i = 0; i < stat->nPiSDFActor; i++) {
                if (stat->actors[i] == pisdfVertexRef) {
                    stat->actorTimes[i][lrtType] += execTime;
                    stat->actorIterations[i][lrtType]++;

                    stat->actorFisrt[i] = std::min(stat->actorFisrt[i], startTimeScaled);
                    stat->actorLast[i] = std::max(stat->actorLast[i], endTimeScaled);
                    break;
                }
            }
            if (i == stat->nPiSDFActor) {
                stat->actors[stat->nPiSDFActor] = vertex->getReference();

                memset(stat->actorTimes[stat->nPiSDFActor], 0, MAX_STATS_PE_TYPES * sizeof(Time));
                memset(stat->actorIterations[stat->nPiSDFActor], 0, MAX_STATS_PE_TYPES * sizeof(Time));

                stat->actorTimes[stat->nPiSDFActor][lrtType] += execTime;
                stat->actorIterations[i][lrtType]++;
                stat->nPiSDFActor++;

                stat->actorFisrt[i] = startTimeScaled;
                stat->actorLast[i] = endTimeScaled;
            }
            break;
        }
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
}

static void writeGanttForSpiderTasks(TraceMessage *message, FILE *ganttFile, FILE *latexFile, ExecutionStat *stat) {
    int i = 0;

    /** Scale the different values of measured time to chip time **/
    Time startTimeScaled = message->getStartTime() / CHIP_FREQ;
    Time endTimeScaled = message->getEndTime() / CHIP_FREQ;
    Time ellapsedTimeScaled = message->getEllapsedTime() / CHIP_FREQ;

    /* Gantt File */
    Platform::get()->fprintf(ganttFile, "\t<event\n");
    Platform::get()->fprintf(ganttFile, "\t\tstart=\"%" PRIu64"\"\n", startTimeScaled);
    Platform::get()->fprintf(ganttFile, "\t\tend=\"%" PRIu64"\"\n", endTimeScaled);
    Platform::get()->fprintf(ganttFile, "\t\ttitle=\"%s\"\n",
                             TimeMonitor::getTaskName((TraceSpiderType) message->getSpiderTask()));
    Platform::get()->fprintf(ganttFile, "\t\tmapping=\"%s\"\n",
                             archi_->getPEFromSpiderID(message->getLRTID())->getName().c_str());
    Platform::get()->fprintf(ganttFile, "\t\tcolor=\"%s\"\n", regenerateColor(i++));
    Platform::get()->fprintf(ganttFile, "\t\t>Step_%d.</event>\n", message->getSpiderTask());

    stat->schedTime = std::max(endTimeScaled, stat->schedTime);

    switch (message->getSpiderTask()) {
        case TRACE_SPIDER_GRAPH:
            stat->graphTime += ellapsedTimeScaled;
            break;
        case TRACE_SPIDER_SCHED:
            stat->mappingTime += ellapsedTimeScaled;
            break;
        case TRACE_SPIDER_OPTIM:
            stat->optimTime += ellapsedTimeScaled;
            break;
        case TRACE_SPIDER_ALLOC:
        default:
            throwSpiderException("Unhandle type of SpiderTrace: %d.", message->getSpiderTask());
    }

    /* Latex File */
    auto latexScaling = 1000.f;
    Platform::get()->fprintf(latexFile, "%f,", startTimeScaled / latexScaling); /* Start */
    Platform::get()->fprintf(latexFile, "%f,", endTimeScaled / latexScaling); /* Duration */
    Platform::get()->fprintf(latexFile, "%" PRIu32",", archi_->getSpiderGRTID()); /* Core index */
    Platform::get()->fprintf(latexFile, "colorSched\n", 15); /* Color */
}

void Spider::printGantt(const char *ganttPath, const char *latexPath, ExecutionStat *stat) {
    FILE *ganttFile = Platform::get()->fopen(ganttPath);
    if (ganttFile == nullptr) {
        throwSpiderException("Failed to open ganttFile.");
    }

    FILE *latexFile = Platform::get()->fopen(latexPath);
    if (latexFile == nullptr) {
        throwSpiderException("Failed to open latexFile.");
    }

    // Writing header
    Platform::get()->fprintf(ganttFile, "<data>\n");
    Platform::get()->fprintf(latexFile, "start,end,core,color\n");

    // Popping data from Trace queue.
    // TODO: change execution stat to proper class
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
    stat->nSRDAGActor = srdag_->getNVertex();
    stat->nSRDAGEdge = srdag_->getNEdge();
    stat->nPiSDFActor = 0;

    stat->memoryUsed = memAlloc_->getMemUsed();

    int n = Launcher::get()->getNLaunched();
    auto *spiderCommunicator = Platform::get()->getSpiderCommunicator();
    while (n) {
        NotificationMessage message;
        spiderCommunicator->pop_notification(Platform::get()->getNLrt(), &message, true);
        if (message.getType() != TRACE_NOTIFICATION) {
            // Push back notification for later
            // We should not have any other kind of notification here though
            spiderCommunicator->push_notification(Platform::get()->getNLrt(), &message);
        } else {
            TraceMessage *msg;
            spiderCommunicator->pop_trace_message(&msg, message.getIndex());
            if (message.getSubType() == TRACE_LRT) {
                writeGanttForVertex(msg, ganttFile, latexFile, stat);
            } else if (message.getSubType() == TRACE_SPIDER) {
                writeGanttForSpiderTasks(msg, ganttFile, latexFile, stat);
            } else {
                throwSpiderException("Unhandled type of Trace: %d", message.getSubType());
            }
            StackMonitor::free(ARCHI_STACK, msg);
        }
        n--;
    }
    Launcher::get()->rstNLaunched();

    Platform::get()->fprintf(ganttFile, "</data>\n");

    Platform::get()->fclose(ganttFile);
    Platform::get()->fclose(latexFile);

    stat->execTime = stat->globalEndTime - stat->schedTime;
}
