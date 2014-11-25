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

#include <spider.h>
#include "Tests.h"

#include <cstdio>
#include <cstdlib>

#define STACK_SIZE (6*1024*1024)

int main(int argc, char* argv[]){
	PiSDFGraph *topPisdf;
	void* memory = malloc(STACK_SIZE);
	StaticStack stack = StaticStack(memory,STACK_SIZE);
	SpiderConfig cfg;
	SRDAGGraph srdag;

	cfg.createSrdag = false;
	cfg.srdag = &srdag;

	printf("Start\n");

//	try{
		for(int i=1; i<=3; i++){
			char name[20];
			sprintf(name, "test0_%d.gv", i);
			stack.free();
			srdag = SRDAGGraph(&stack);
			topPisdf = initPisdf_test0(&stack, i);
			jit_ms(topPisdf, &cfg);
			srdag.print(name);
		}


		for(int i=1; i<=3; i++){
			char name[20];
			sprintf(name, "test1_%d.gv", i);
			stack.free();
			srdag = SRDAGGraph(&stack);
			topPisdf = initPisdf_test1(&stack, i);
			jit_ms(topPisdf, &cfg);
			srdag.print(name);
		}

		for(int i=1; i<=3; i++){
			char name[20];
			sprintf(name, "test2_%d.gv", i);
			stack.free();
			srdag = SRDAGGraph(&stack);
			topPisdf = initPisdf_test2(&stack, i);
			jit_ms(topPisdf, &cfg);
			srdag.print(name);
		}

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test3(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test3.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test4(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test4.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test5(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test5.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test6(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test6.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test7(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test7.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test8(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test8.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_test9(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("test9.gv");

		stack.free();
		srdag = SRDAGGraph(&stack);
		topPisdf = initPisdf_testA(&stack);
		jit_ms(topPisdf, &cfg);
		srdag.print("testA.gv");
//
//	}catch(const char* s){
//		printf("Exception : %s\n", s);
//	}
	printf("finished\n");

	free(memory);

	return 0;
}
