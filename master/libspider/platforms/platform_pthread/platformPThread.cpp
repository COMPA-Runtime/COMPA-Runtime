/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2017 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2018)
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
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>

#endif // _WIN32

#ifdef _MSC_VER
#define steady_clock system_clock

#if (_MSC_VER < 1900)
#define snprintf _snprintf
#endif
#endif

#include <pthread.h>


#include <cstdio>

#include <platformPThread.h>

#include <cstdarg>

#include <graphs/Archi/SharedMemArchi.h>

#include <lrt.h>
#include <PThreadSpiderCommunicator.h>
#include <spider.h>
#include <tools/Rational.h>
#include "ControlMessageQueue.h"

#define PLATFORM_FPRINTF_BUFFERSIZE 200

#define MAX_MSG_SIZE 10*1024

static char buffer[PLATFORM_FPRINTF_BUFFERSIZE];
static struct timespec start;

static void *dataMem;

static std::chrono::time_point<std::chrono::steady_clock> start_steady;

static auto origin_steady = std::chrono::steady_clock::now();

static SharedMemArchi *archi_;


pthread_barrier_t pthreadLRTBarrier;


void printfSpider();

static void setAffinity(int cpuId) {
#ifdef WIN32
    fprintf(stdout, "CPU affinity is not supported on Windows platforms. Ignoring argument %d.\n", cpuId);
#else
    cpu_set_t mask;
    int status;

    CPU_ZERO(&mask);
    CPU_SET(cpuId, &mask);
    status = pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
    if (status != 0) {
        perror("sched_setaffinity");
    }
#endif
}


void *lrtPthreadRunner(void *args) {
    auto lrtInfo = (LRTInfo *) (args);
    /** Registering LRT */
    pthread_t self = pthread_self();
    lrtInfo->platform->registerLRT(lrtInfo->lrtID, self);

    /** Waiting for every threads to register itself */
    pthread_barrier_wait(lrtInfo->pthreadBarrier);

    /** Initialize the LRT specific stack */
    StackMonitor::initStack(LRT_STACK, lrtInfo->lrtStack);

    /** Set core affinity (if supported by the OS) */
    setAffinity(lrtInfo->coreAffinity);

    /** Set the function table */
    lrtInfo->lrt->setFctTbl(lrtInfo->fcts, lrtInfo->nFcts);
#ifdef PAPI_AVAILABLE
    /** Enable PAPIFY if needed to */
    if (lrtInfo->usePapify) {
        auto papifyJobInfo = lrtInfo->platform->getPapifyInfo();
        lrtInfo->lrt->setUsePapify();
        for (auto &mapEntry : papifyJobInfo) {
            lrtInfo->lrt->addPapifyJobInfo(mapEntry.first, new PapifyAction(*mapEntry.second, lrtInfo->lrtID));
        }
    }
#endif
    /** Wait for all LRTs to be created */
    pthread_barrier_wait(lrtInfo->pthreadBarrier);
    /** Run LRT */
    lrtInfo->lrt->runInfinitly();
    /** Cleaning LRT specific stacks */
    StackMonitor::freeAll(LRT_STACK);
    StackMonitor::clean(LRT_STACK);
    /** Wait for all LRTs to finish */
    pthread_barrier_wait(lrtInfo->pthreadBarrier);
    /** Exit thread */
    pthread_exit(EXIT_SUCCESS);
}

static LRTInfo *lrtInfoArray = nullptr;

PlatformPThread::PlatformPThread(SpiderConfig &config) {
    if (platform_) {
        throw std::runtime_error("ERROR: A platform already exist.");
    }
    platform_ = this;


    nLrt_ = (unsigned int) config.platform.nLrt;
    /** Init of the different stacks **/
    initStacks(config);

    stackLrt = CREATE_MUL(ARCHI_STACK, nLrt_, Stack*);

    lrt_ = CREATE_MUL(ARCHI_STACK, nLrt_, LRT*);
    lrtCom_ = CREATE_MUL(ARCHI_STACK, nLrt_, LrtCommunicator*);
    lrtThreadsArray = CREATE_MUL(ARCHI_STACK, nLrt_, pthread_t);


    /** Create the different queues */
    spider2LrtJobQueue_ = CREATE(ARCHI_STACK, ControlMessageQueue<JobMessage *>);
    lrt2SpiderParamQueue_ = CREATE(ARCHI_STACK, ControlMessageQueue<ParameterMessage *>);
    lrtNotificationQueues_ = CREATE_MUL(ARCHI_STACK, nLrt_ + 1, NotificationQueue*);

    for (unsigned int i = 0; i < nLrt_ + 1; ++i) {
        lrtNotificationQueues_[i] = CREATE(ARCHI_STACK, NotificationQueue);
    }

    /** FIFOs allocation */
    dataQueues_ = CREATE(ARCHI_STACK, DataQueues)(nLrt_);
    /** TraceQueue allocation */
    traceQueue_ = CREATE(ARCHI_STACK, TraceQueue)(MAX_MSG_SIZE, nLrt_);
    /** Threads structure */
    thread_lrt_ = CREATE_MUL(ARCHI_STACK, nLrt_ - 1, pthread_t);
    lrtInfoArray = CREATE_MUL(ARCHI_STACK, nLrt_ - 1, LRTInfo);

    // TODO use "usePapify" only for monitored LRTs / HW PEs
#ifndef PAPI_AVAILABLE
    // If PAPI is not available on the current platform_, force disable it
    if (config.usePapify) {
        printf("WARNING: Spider was not compiled on a platform_ with PAPI, thus the monitoring is disabled.\n");
    }
    config.usePapify = false;
#else
    if (config.usePapify) {
        // Initializing Papify
        PapifyEventLib *papifyEventLib = new PapifyEventLib();

        // Register Papify actor configuration
        if (config.usePapify) {
            std::map<lrtFct, PapifyConfig *>::iterator it;
            for (it = config.papifyJobInfo.begin(); it != config.papifyJobInfo.end(); ++it) {
                PapifyConfig *papifyConfig = it->second;
                PapifyAction *papifyAction = new PapifyAction(
                        /* componentName */   papifyConfig->peType_,
                        /* PEName */          papifyConfig->peID_,
                        /* actorName */       papifyConfig->actorName_,
                        /* num_events */      papifyConfig->eventSize_,
                        /* all_events_name */ papifyConfig->monitoredEvents_,
                        /* eventSet_Id */     papifyConfig->eventSetID_, papifyConfig->isTiming_, papifyEventLib);
                papifyJobInfo.insert(std::make_pair(it->first, papifyAction));
            }
        }
    }
#endif

    // Find LCM of share memory size and minAllocSize
    auto minAlignedSharedMemory = Rational::compute_lcm(config.platform.shMemSize, getpagesize());
    dataMem = operator new((size_t) minAlignedSharedMemory);

    /** Filling up parameters for each threads */
    pthread_barrier_init(&pthreadLRTBarrier, nullptr, nLrt_);
    int offsetPe = 0;
    for (int pe = 0; pe < config.platform.nPeType; ++pe) {
        for (int i = 0; i < config.platform.pesPerPeType[pe]; i++) {


            lrtCom_[i + offsetPe] = CREATE(ARCHI_STACK, PThreadLrtCommunicator)(
                    spider2LrtJobQueue_,
                    lrtNotificationQueues_[i + offsetPe],
                    dataQueues_,
                    traceQueue_);

            lrt_[i + offsetPe] = CREATE(ARCHI_STACK, LRT)(i);

            lrtInfoArray[i + offsetPe].lrt = lrt_[i + offsetPe];
            lrtInfoArray[i + offsetPe].fcts = config.platform.fcts;
            lrtInfoArray[i + offsetPe].nFcts = config.platform.nLrtFcts;
            lrtInfoArray[i + offsetPe].lrtID = i + offsetPe;
            lrtInfoArray[i + offsetPe].platform = this;
            lrtInfoArray[i + offsetPe].coreAffinity = config.platform.coreAffinities[pe][i];
            lrtInfoArray[i + offsetPe].pthreadBarrier = &pthreadLRTBarrier;
            /** Stack related information */
            lrtInfoArray[i + offsetPe].lrtStack.name = config.lrtStack.name;
            lrtInfoArray[i + offsetPe].lrtStack.type = config.lrtStack.type;
            lrtInfoArray[i + offsetPe].lrtStack.start = (void *) ((char *) config.lrtStack.start +
                                                                  (i + offsetPe) * config.lrtStack.size / nLrt_);
            lrtInfoArray[i + offsetPe].lrtStack.size = config.lrtStack.size / nLrt_;
            /** Papify related information */
            lrtInfoArray[i + offsetPe].usePapify = config.usePapify;
        }
    }

    lrtThreadsArray[0] = pthread_self();

    /** Starting the threads */
    for (unsigned int i = 1; i < nLrt_; i++) {
        pthread_create(&thread_lrt_[i - 1], nullptr, &lrtPthreadRunner, &lrtInfoArray[i]);
    }

    //waiting for every threads to register itself in lrtThreadsArray
    pthread_barrier_wait(&pthreadLRTBarrier);

    //Declaration des stacks spécific au thread
    StackConfig lrtStackConfig;
    lrtStackConfig.name = config.lrtStack.name;
    lrtStackConfig.type = config.lrtStack.type;
    lrtStackConfig.start = config.lrtStack.start;
    lrtStackConfig.size = config.lrtStack.size / nLrt_;
    StackMonitor::initStack(LRT_STACK, lrtStackConfig);

    /** Initialize shared memory */
    memset(dataMem, 0, (size_t) minAlignedSharedMemory);

    /** Initialize LRT and Communicators */
    spiderCom_ = CREATE(ARCHI_STACK, PThreadSpiderCommunicator)(
            spider2LrtJobQueue_,
            lrt2SpiderParamQueue_,
            lrtNotificationQueues_,
            traceQueue_);

    // Check papify profiles
#ifdef PAPI_AVAILABLE
    if (config.usePapify) {
        lrt_[0]->setUsePapify();
        std::map<lrtFct, PapifyAction *>::iterator it;
        for (it = papifyJobInfo.begin(); it != papifyJobInfo.end(); ++it) {
            lrt_[0]->addPapifyJobInfo(it->first, it->second);
        }
    }
#endif

    // Wait for all LRTs to be ready to start
    pthread_barrier_wait(&pthreadLRTBarrier);


    setAffinity(0);
    lrt_[0]->setFctTbl(config.platform.fcts, config.platform.nLrtFcts);


    /** Create Archi */
    int mainPE = 0;
    int mainPEType = 0;
    archi_ = CREATE(ARCHI_STACK, SharedMemArchi)(
            /* Nb PE */     nLrt_,
            /* Nb PE Type*/ config.platform.nPeType,
            /* Spider Pe */ mainPE,
            /*MappingTime*/ this->mappingTime);

    archi_->setPEType(mainPE, mainPEType);
    archi_->activatePE(mainPE);

    char name[40];
    sprintf(name, "TID %ld (Spider)", lrtThreadsArray[0]);
    archi_->setName(mainPE, name);
    offsetPe = 0;
    for (int pe = 0; pe < config.platform.nPeType; ++pe) {
        archi_->setPETypeRecvSpeed(pe, 1, 10);
        archi_->setPETypeSendSpeed(pe, 1, 10);
        for (int i = 0; i < config.platform.pesPerPeType[pe]; i++) {
            if (pe == mainPEType && (i + offsetPe) == mainPE) {
                continue;
            }
            sprintf(name, "TID %ld (LRT %d)", lrtThreadsArray[i + offsetPe], i + offsetPe);
            archi_->setPEType(i + offsetPe, pe);
            archi_->setName(i + offsetPe, name);
            archi_->activatePE(i + offsetPe);
        }
        offsetPe += config.platform.pesPerPeType[pe];
    }

    Spider::setArchi(archi_);

    //this->rstTime();
}

PlatformPThread::~PlatformPThread() {
    auto spiderCommunicator = getSpiderCommunicator();
    for (unsigned int i = 1; i < nLrt_; ++i) {
        NotificationMessage message(LRT_NOTIFICATION, LRT_STOP);
        spiderCommunicator->push_notification(i, &message);
    }

    //wait for every LRT to end
    pthread_barrier_wait(&pthreadLRTBarrier);


    //wait for each thread to free its lrt and archi stacks and to reach its end
    for (unsigned int i = 1; i < nLrt_; i++) {
        pthread_join(lrtThreadsArray[i], nullptr);
    }

#ifdef PAPI_AVAILABLE
    /** Free Papify information */
    if (!papifyJobInfo.empty()) {
        std::map<lrtFct, PapifyAction *>::iterator it;
        // Delete the event lib manager
        delete papifyJobInfo.begin()->second->getPapifyEventLib();
    }
#endif

    for (unsigned int i = 0; i < nLrt_; i++) {
        lrt_[i]->~LRT();
        StackMonitor::free(ARCHI_STACK, lrt_[i]);
        StackMonitor::free(ARCHI_STACK, lrtCom_[i]);
    }
    StackMonitor::free(ARCHI_STACK, spiderCom_);


    archi_->~SharedMemArchi();

    StackMonitor::free(ARCHI_STACK, archi_);
    StackMonitor::free(ARCHI_STACK, thread_lrt_);
    StackMonitor::free(ARCHI_STACK, lrtInfoArray);

    /** Freeing queues **/
    for (unsigned int i = 0; i < nLrt_ + 1; ++i) {
        lrtNotificationQueues_[i]->~NotificationQueue();
        StackMonitor::free(ARCHI_STACK, lrtNotificationQueues_[i]);
    }
    spider2LrtJobQueue_->~ControlMessageQueue<JobMessage *>();
    lrt2SpiderParamQueue_->~ControlMessageQueue<ParameterMessage *>();
    StackMonitor::free(ARCHI_STACK, lrtNotificationQueues_);
    StackMonitor::free(ARCHI_STACK, spider2LrtJobQueue_);
    StackMonitor::free(ARCHI_STACK, lrt2SpiderParamQueue_);


    dataQueues_->~DataQueues();
    traceQueue_->~TraceQueue();

    StackMonitor::free(ARCHI_STACK, dataQueues_);
    StackMonitor::free(ARCHI_STACK, traceQueue_);


    //Desallocation des tableaux dynamiques
    StackMonitor::free(ARCHI_STACK, lrt_);
    StackMonitor::free(ARCHI_STACK, lrtCom_);

    StackMonitor::freeAll(LRT_STACK);
    StackMonitor::clean(LRT_STACK);
    StackMonitor::free(ARCHI_STACK, stackLrt);

    StackMonitor::free(ARCHI_STACK, lrtThreadsArray);

    StackMonitor::freeAll(ARCHI_STACK);
    StackMonitor::freeAll(TRANSFO_STACK);
    StackMonitor::freeAll(SRDAG_STACK);
    StackMonitor::freeAll(PISDF_STACK);

    //WARNING : Thread specific stacks have to be cleaned BEFORE exiting threads
    StackMonitor::clean(ARCHI_STACK);
    StackMonitor::clean(TRANSFO_STACK);
    StackMonitor::clean(SRDAG_STACK);
    StackMonitor::clean(PISDF_STACK);

    //Destroying synchronisation barrier
    pthread_barrier_destroy(&pthreadLRTBarrier);

    operator delete(dataMem);
}

/** File Handling */
FILE *PlatformPThread::fopen(const char *name) {
    return std::fopen(name, "w+");
}

void PlatformPThread::fprintf(FILE *id, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

#ifdef _WIN32
    int n = _vsnprintf(buffer, PLATFORM_FPRINTF_BUFFERSIZE, fmt, ap);
#else
    int n = vsnprintf(buffer, PLATFORM_FPRINTF_BUFFERSIZE, fmt, ap);
#endif

    if (n >= PLATFORM_FPRINTF_BUFFERSIZE) {
        printf("PLATFORM_FPRINTF_BUFFERSIZE too small\n");
    }

    for (int i = 0; i < n; i++) fputc(buffer[i], id);
}

void PlatformPThread::fclose(FILE *id) {
    if (id != nullptr) {
        std::fclose(id);
        id = nullptr;
    }
}

void *PlatformPThread::virt_to_phy(void *address) {
    return (void *) ((long) dataMem + (long) address);
}

int PlatformPThread::getCacheLineSize() {
    return 0;
}

int PlatformPThread::getMinAllocSize() {
#ifdef _WIN32
    //workaround because Windows
    return 4096;
#else
    return getpagesize();
#endif
}

void PlatformPThread::rstJobIxSend() {
    //Sending a msg to all slave LRTs, end of graph iteration
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        auto msg = (Message *) getSpiderCommunicator()->ctrl_start_send(i, sizeof(Message));
//        msg->id_ = MSG_END_ITER;
//        getSpiderCommunicator()->ctrl_end_send(i, sizeof(Message));
//    }
//
//    //Waiting for slave LRTs to finish their job queue
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        void *msg = nullptr;
//
//        do {
//            getSpiderCommunicator()->ctrl_start_recv_block(i, &msg);
//            if (((Message *) msg)->id_ == MSG_END_ITER)
//                break;
//            else
//                getSpiderCommunicator()->ctrl_end_recv(i);
//        } while (true);
//        getSpiderCommunicator()->ctrl_end_recv(i);
//    }
//
//    //Sending a msg to all slave LRTs, reset jobIx counter
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        auto msg = (Message *) getSpiderCommunicator()->ctrl_start_send(i, sizeof(Message));
//        msg->id_ = MSG_RESET_LRT;
//        getSpiderCommunicator()->ctrl_end_send(i, sizeof(Message));
//    }
}

void PlatformPThread::rstJobIxRecv() {
    auto spiderCommunicator = Platform::get()->getSpiderCommunicator();
    NotificationMessage clearJobMessage(JOB_NOTIFICATION, JOB_CLEAR_QUEUE);
    for (unsigned int i = 0; i < nLrt_; ++i) {
        NotificationMessage finishedMessage;
        /** Wait for LRTs to finish their jobs **/
        while (true) {
            spiderCommunicator->pop_notification(Platform::get()->getNLrt(), &finishedMessage, true);
            if (finishedMessage.getType() == LRT_NOTIFICATION &&
                finishedMessage.getSubType() == LRT_FINISHED_ITERATION) {
#ifdef VERBOSE_JOBS
                fprintf(stderr, "INFO: LRT: %d -- received end signal.\n", finishedMessage.getIndex());
#endif
                /** Send message to clear job queue **/
                spiderCommunicator->push_notification(finishedMessage.getIndex(), &clearJobMessage);
                break;
            }
        }
    }
}

void PlatformPThread::rstJobIx() {
//    //Sending a msg to all slave LRTs, end of graph iteration
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        auto msg = (Message *) getSpiderCommunicator()->ctrl_start_send(i, sizeof(Message));
//        msg->id_ = MSG_END_ITER;
//        getSpiderCommunicator()->ctrl_end_send(i, sizeof(Message));
//    }
//
//    //Waiting for slave LRTs to finish their job queue
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        void *msg = nullptr;
//
//        do {
//            getSpiderCommunicator()->ctrl_start_recv_block(i, &msg);
//            if (((Message *) msg)->id_ == MSG_END_ITER)
//                break;
//            else
//                getSpiderCommunicator()->ctrl_end_recv(i);
//        } while (true);
//        getSpiderCommunicator()->ctrl_end_recv(i);
//    }
//
//    //Sending a msg to all slave LRTs, reset jobIx counter
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        auto msg = (Message *) getSpiderCommunicator()->ctrl_start_send(i, sizeof(Message));
//        msg->id_ = MSG_RESET_LRT;
//        getSpiderCommunicator()->ctrl_end_send(i, sizeof(Message));
//    }
//
//    //reseting master LRT jobIx counter
//    Platform::get()->getLrt()->setJobIx(-1);
//
//    //reseting jobTab
//    for (unsigned int i = 0; i < nLrt_; i++) {
//        lrtCom_[0]->setLrtJobIx(i, -1);
//    }
//
//    //Waiting for slave LRTs to reset their jobIx counter
//    for (unsigned int i = 1; i < nLrt_; i++) {
//        void *msg = nullptr;
//        do {
//            getSpiderCommunicator()->ctrl_start_recv_block(i, &msg);
//            if (((Message *) msg)->id_ == MSG_RESET_LRT)
//                break;
//            else
//                getSpiderCommunicator()->ctrl_end_recv(i);
//        } while (true);
//        getSpiderCommunicator()->ctrl_end_recv(i);
//    }
}

/** Time Handling */
void PlatformPThread::rstTime(ClearTimeMessage *msg) {
    start = msg->timespec_;
}

void PlatformPThread::rstTime() {
//    start_steady = std::chrono::steady_clock::now();
//
//    start.tv_sec = (start_steady - origin_steady).count() / 1000000000;
//    start.tv_nsec = (start_steady - origin_steady).count() - (start_steady - origin_steady).count() / 1000000000;
//
//
//    for (int lrt = 1; lrt < archi_->getNPE(); lrt++) {
//        auto msg = (ClearTimeMessage *) getSpiderCommunicator()->ctrl_start_send(lrt, sizeof(ClearTimeMessage));
//        msg->id_ = MSG_CLEAR_TIME;
//        msg->timespec_ = start;
//        getSpiderCommunicator()->ctrl_end_send(lrt, sizeof(ClearTimeMessage));
//    }
}

Time PlatformPThread::getTime() {
    std::chrono::time_point<std::chrono::steady_clock> ts_steady = std::chrono::steady_clock::now();
    long long val_steady = (ts_steady - start_steady).count();

#ifdef _WIN32
    // Spider will think something went bad if returned time is 0, so in such case we're setting it to 1 because time in Windows is bad
    if (val_steady == 0){
        val_steady++;
    }
#endif // _WIN32

    return val_steady;
}

Time PlatformPThread::mappingTime(int nActors, int /*nPe*/) {
    return (Time) 1 * nActors;
}

void PlatformPThread::initStacks(SpiderConfig &config) {
    stackPisdf = nullptr;
    stackSrdag = nullptr;
    stackTransfo = nullptr;
    stackArchi = nullptr;

    /** Global stacks initialisation */
    StackMonitor::initStack(PISDF_STACK, config.pisdfStack);
    StackMonitor::initStack(SRDAG_STACK, config.srdagStack);
    StackMonitor::initStack(TRANSFO_STACK, config.transfoStack);
    StackMonitor::initStack(ARCHI_STACK, config.archiStack);
}

