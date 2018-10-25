/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2018)
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
#ifndef PLATFORM_PTHREAD_H
#define PLATFORM_PTHREAD_H

#include "platform.h"
#include <signal.h>

#include "TraceQueue.h"
#include "ControlQueue.h"

#ifdef PAPI_AVAILABLE
#include "../papify/PapifyAction.h"
#endif

// semaphore.h includes _ptw32.h that redefines types int64_t and uint64_t on Visual Studio,
// making compilation error with the IDE's own declaration of said types
#include <semaphore.h>

#ifdef _MSC_VER
#ifdef int64_t
#undef int64_t
#endif

#ifdef uint64_t
#undef uint64_t
#endif
#endif

#include <queue>
#include <pthread.h>
#include <PThreadLrtCommunicator.h>

#include <map>

struct Arg_lrt;

class PlatformPThread : public Platform {
public:
    /** File Handling */
    virtual FILE *fopen(const char *name);

    virtual void fprintf(FILE *id, const char *fmt, ...);

    virtual void fclose(FILE *id);

    /** Shared Memory Handling */
    virtual void *virt_to_phy(void *address);

    virtual int getMinAllocSize();

    virtual int getCacheLineSize();

    /** Time Handling */
    virtual void rstTime();

    virtual void rstTime(struct ClearTimeMsg *msg);

    virtual Time getTime();

    virtual void rstJobIx();

    /** Platform getter/setter */
    inline LRT *getLrt();

    inline int getLrtIx();

    inline int getNLrt();

    inline LrtCommunicator *getLrtCommunicator();

    inline SpiderCommunicator *getSpiderCommunicator();

    inline void setStack(SpiderStack id, Stack *stack);

    inline Stack *getStack(SpiderStack id);

    inline int getThreadNumber();

    /* Fonction de thread */
    void lrtPThread(Arg_lrt *argument_lrt);

    explicit PlatformPThread(SpiderConfig &config);

    virtual ~PlatformPThread();


private:
    static Time mappingTime(int nActors, int nPe);

    int nLrt_;
    pthread_t *thread_ID_tab_;

    //Pointeurs vers les stacks
    Stack *stackPisdf;
    Stack *stackSrdag;
    Stack *stackTransfo;
    Stack *stackArchi;
    Stack **stackLrt;

    ControlQueue **spider2LrtQueues_;
    ControlQueue **lrt2SpiderQueues_;
    DataQueues *dataQueues_;
    TraceQueue *traceQueue_;

    LRT **lrt_;
    LrtCommunicator **lrtCom_;
    SpiderCommunicator *spiderCom_;

    pthread_t *thread_lrt_;
    Arg_lrt *arg_lrt_;

#ifdef PAPI_AVAILABLE
    // Papify information
    std::map<lrtFct, PapifyAction *> papifyJobInfo;
#endif
};


inline void PlatformPThread::setStack(SpiderStack id, Stack *stack) {
    switch (id) {
        case PISDF_STACK :
            stackPisdf = stack;
            break;
        case SRDAG_STACK :
            stackSrdag = stack;
            break;
        case TRANSFO_STACK :
            stackTransfo = stack;
            break;
        case ARCHI_STACK :
            stackArchi = stack;
            break;
        case LRT_STACK :
            stackLrt[getThreadNumber()] = stack;
            break;
        default :
            throw std::runtime_error("Error in stack index\n");
    }
}

inline Stack *PlatformPThread::getStack(SpiderStack id) {
    switch (id) {
        case PISDF_STACK :
            return stackPisdf;
            break;
        case SRDAG_STACK :
            return stackSrdag;
            break;
        case TRANSFO_STACK :
            return stackTransfo;
            break;
        case ARCHI_STACK :
            return stackArchi;
            break;
        case LRT_STACK :
            return stackLrt[getThreadNumber()];
            break;
        default :
            throw std::runtime_error("Error in stack index\n");
    }
}

inline int PlatformPThread::getThreadNumber() {
    for (int i = 0; i < nLrt_; i++) {
        if (pthread_equal(thread_ID_tab_[i], pthread_self()) != 0)
            return i;
    }
    throw std::runtime_error("Error undefined ID\n");
}

inline LRT *PlatformPThread::getLrt() {
    return lrt_[getThreadNumber()];
}

inline int PlatformPThread::getLrtIx() {
    return getThreadNumber();
}

inline int PlatformPThread::getNLrt() {
    return nLrt_;
}

inline LrtCommunicator *PlatformPThread::getLrtCommunicator() {
    return lrtCom_[getThreadNumber()];
}

inline SpiderCommunicator *PlatformPThread::getSpiderCommunicator() {
    if (spiderCom_)
        return spiderCom_;
    else
        throw std::runtime_error("Error undefined spider communicator\n");
}


// Structure de passage d'argument dans le thread
typedef struct Arg_lrt {
    PlatformPThread *instance;
    LrtCommunicator *lrtCom;
    LRT *lrt;
    int shMemSize;
    lrtFct *fcts;
    int nLrtFcts;
    int index;
    int nLrt;
    StackConfig lrtStack;
    bool usePapify;
    int coreAffinity;
} Arg_lrt;

// Fonction wrapper pour lancer un thread sur une méthode d'objet
void *lrtPThread_helper(void *voidArgs);


#endif/*PLATFORM_PTHREADS_H*/
