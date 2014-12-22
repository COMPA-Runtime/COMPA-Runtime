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

#include <tools/DynStack.h>
#include <cstdio>
#include <algorithm>

#include <cstdlib>
#include <malloc.h>

DynStack::DynStack(const char* name): Stack(name){
	curUsedSize_ = 0;
	maxSize_ = 0;
	nb_ = 0;
}

DynStack::~DynStack(){
	printStat();
}

void *DynStack::alloc(int size){
	curUsedSize_ += size;
	maxSize_ = std::max(maxSize_, curUsedSize_);
	nb_++;
	return std::malloc(size);
}

void DynStack::freeAll(){
	if(nb_ != 0){
		printf("DynStack Warning (%s): FreeAll called with %d allocated item\n", getName(), nb_);
	}
}

void DynStack::free(void* var){
	int size = malloc_usable_size (var);
	maxSize_ = std::max(maxSize_, curUsedSize_);
	curUsedSize_ -= size;
	if(size == 0){
		printf("Error %s free'd already free'd memory\n", getName());
	}
	std::free(var);
	nb_--;
}

void DynStack::printStat(){
	printf("%s: ", getName());

	if(maxSize_ < 1024)
		printf("\t%lld B", maxSize_);
	else if(maxSize_ < 1024*1024)
		printf("\t%.1f KB", maxSize_/1024.);
	else if(maxSize_ < 1024*1024*1024)
		printf("\t%.1f MB", maxSize_/1024./1024.);
	else
		printf("\t%.1f GB", maxSize_/1024./1024./1024.);

	if(nb_)
		printf(", \t%d still in use", nb_);

	printf("\n");
}
