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
#include "PThreadSpiderCommunicator.h"

#include <platform.h>

//PThreadSpiderCommunicator::PThreadSpiderCommunicator(ControlQueue **spider2LrtQueues,
//                                                     ControlQueue **lrt2SpiderQueues,
//                                                     TraceQueue *traceQueue, int nLrt) {
//    spider2LrtQueues_ = spider2LrtQueues;
//    lrt2SpiderQueues_ = lrt2SpiderQueues;
//    traceQueue_ = traceQueue;
//    nLrt_ = nLrt;
//}

PThreadSpiderCommunicator::~PThreadSpiderCommunicator() {
}

//void PThreadSpiderCommunicator::rst_ctrl_queue() {
//    fprintf(stderr, "nLrt: %d\n", nLrt_);
//    for (int i = 0; i < nLrt_; ++i) {
//        spider2LrtQueues_[i]->rst();
//    }
//}

//void *PThreadSpiderCommunicator::ctrl_start_send(int lrtIx, std::uint64_t size) {
//    return spider2LrtQueues_[lrtIx]->push_start(size);
//}
//
//void PThreadSpiderCommunicator::ctrl_end_send(int lrtIx, std::uint64_t size) {
//    return spider2LrtQueues_[lrtIx]->push_end(size);
//}
//
//std::uint64_t PThreadSpiderCommunicator::ctrl_start_recv(int lrtIx, void **data) {
//    return lrt2SpiderQueues_[lrtIx]->pop_start(data, false);
//}
//
//void PThreadSpiderCommunicator::ctrl_start_recv_block(int lrtIx, void **data) {
//    lrt2SpiderQueues_[lrtIx]->pop_start(data, true);
//}
//
//void PThreadSpiderCommunicator::ctrl_end_recv(int lrtIx) {
//    return lrt2SpiderQueues_[lrtIx]->pop_end();
//}

PThreadSpiderCommunicator::PThreadSpiderCommunicator(ControlMessageQueue<JobMessage *> *spider2LrtJobQueue,
                                                     ControlMessageQueue<LRTMessage *> *spider2LrtLRTQueue,
                                                     NotificationQueue **notificationQueue, TraceQueue *traceQueue,
                                                     int nLrt) {
    spider2LrtJobQueue_ = spider2LrtJobQueue;
    spider2LrtLRTQueue_ = spider2LrtLRTQueue;
    notificationQueue_ = notificationQueue;
    traceQueue_ = traceQueue;
    nLrt_ = nLrt;
}

bool PThreadSpiderCommunicator::popNotification(int lrtID, NotificationMessage *msg, bool blocking) {
    return notificationQueue_[lrtID]->pop(msg, blocking);
}

void PThreadSpiderCommunicator::pushNotification(int lrtID, NotificationMessage *msg) {
    notificationQueue_[lrtID]->push(msg);
}

void *PThreadSpiderCommunicator::trace_start_send(int size) {
    return traceQueue_->push_start(Platform::get()->getNLrt(), size);
}

void PThreadSpiderCommunicator::trace_end_send(int size) {
    return traceQueue_->push_end(Platform::get()->getNLrt(), size);
}

int PThreadSpiderCommunicator::trace_start_recv(void **data) {
    return traceQueue_->pop_start(data, false);
}

void PThreadSpiderCommunicator::trace_start_recv_block(void **data) {
    traceQueue_->pop_start(data, true);
}

void PThreadSpiderCommunicator::trace_end_recv() {
    return traceQueue_->pop_end();
}

std::int32_t PThreadSpiderCommunicator::pushJobMessage(JobMessage **message) {
    return spider2LrtJobQueue_->push(message);
}

std::int32_t PThreadSpiderCommunicator::pushLRTMessage(LRTMessage **message) {
    return spider2LrtLRTQueue_->push(message);
}


