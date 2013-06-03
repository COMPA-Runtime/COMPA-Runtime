/*
 * Copyright (c) 2009 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

/*
 * helloworld.c: simple test application
 */


#include "platform.h"
#include "xparameters.h"
#include "lrt_prototypes.h"
#include "xuartlite_l.h"
#include "xgpio_l.h"
#include "lrt.h"


#define DOMAIN			0
#define SIZE			63
//#define SIZE			512
#define PERF_MON		0
#define PRINT_VALUES	0


#if PERF_MON == 1
#include "xaxipmon_hw.h"
#endif


//#include "xintc_l.h"

static INT8U buffer_in[SIZE];


void action1()
{
	print("Hello from action 1 \n");

	INT8U error;
	INT32U i;
//	INT16U buffer_in[SIZE];

	OS_TCB tcb;
	error = OSTaskQuery(OS_PRIO_SELF, &tcb);

	if(error == OS_ERR_NONE)
	{
		for(i=0;i<SIZE;i++)
			buffer_in[i] = i+1;


	#if PERF_MON == 1
		XAxiPmon_WriteReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_CTL_OFFSET, XAPM_CR_GCC_RESET_MASK);
		XAxiPmon_WriteReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_CTL_OFFSET, XAPM_CR_GCC_ENABLE_MASK);
	#endif

//		blocking_write_output_fifo(tcb.fifo_out[0], SIZE, (INT8U*)buffer_in, &error);
		write_output_fifo(tcb.fifo_out[0], SIZE, buffer_in, &error);

	#if PERF_MON == 1
		INT32U nb_cycles = XAxiPmon_ReadReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_GCC_LOW_OFFSET);
		print("cycles for writing,");putnum_dec(nb_cycles);print("\n");
	#endif
	//		XGpio_WriteReg(XPAR_LEDS_4BITS_BASEADDR, XGPIO_DATA_OFFSET, 0xFF);
		if(error != OS_ERR_NONE)
		{
			print("Error: "); putnum_dec(error); print("\n");
		}
	}
}



void action3()
{
	print("Hello from action 3 \n");

	INT8U buffer_out[SIZE];
	INT8U error;

	OS_TCB tcb;
	error = OSTaskQuery(OS_PRIO_SELF, &tcb);

	if(error == OS_ERR_NONE)
	{
	#if PERF_MON == 1
		XAxiPmon_WriteReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_CTL_OFFSET, XAPM_CR_GCC_RESET_MASK);
		XAxiPmon_WriteReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_CTL_OFFSET, XAPM_CR_GCC_ENABLE_MASK);
	#endif

//		blocking_read_input_fifo(tcb.fifo_in[0], SIZE, buffer_out, &error);
		read_input_fifo(tcb.fifo_in[0], SIZE, buffer_out, &error);

	#if PERF_MON == 1
		INT32U nb_cycles = XAxiPmon_ReadReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_GCC_LOW_OFFSET);
		print("cycles for reading,");putnum_dec(nb_cycles);print("\n");
	#endif

		if(error == OS_ERR_NONE)
		{
	#if PRINT_VALUES == 1
			print("Read values: \n");
			INT32U i;
			for(i=0;i<SIZE;i++)
			{
				putnum_dec(buffer_out[i]); print("\n");
			}
	#endif
		}
		else
		{
			print("Error: "); putnum_dec(error); print("\n");
		}

	}
}






void action2()
{
	print("Hello from action 2 \n");

	INT8U error;
	INT8U buffer_in[SIZE];

	OS_TCB tcb;
	error = OSTaskQuery(OS_PRIO_SELF, &tcb);

	if(error == OS_ERR_NONE)
	{

	#if PERF_MON == 1
		print("bytes,cycles\n");
		INT32U nb_cycles;
	#endif

	#if PERF_MON == 1
		XAxiPmon_WriteReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_CTL_OFFSET, XAPM_CR_GCC_RESET_MASK);
		XAxiPmon_WriteReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_CTL_OFFSET, XAPM_CR_GCC_ENABLE_MASK);
	#endif

		read_input_fifo(tcb.fifo_in[0], SIZE, buffer_in, &error);

		write_output_fifo(tcb.fifo_out[0], SIZE, buffer_in, &error);

	#if PERF_MON == 1
		nb_cycles = XAxiPmon_ReadReg(XPAR_AXI_PERF_MON_0_BASEADDR, XAPM_GCC_LOW_OFFSET);
		putnum_dec(size); print(","); putnum_dec(nb_cycles); print("\n");
	#endif

		if(error == OS_ERR_NONE)
		{
	#if PRINT_VALUES == 1
			print("Read values: \n");
			INT32U i;
			for(i=0;i<SIZE;i++)
			{
				putnum_dec(buffer_out[i]); print("\n");
			}
	#endif
		}
		else
		{
			print("Error: "); putnum_dec(error); print("\n");
		}
	}
}




int main()
{
    init_platform();

	/*
	 * Enable interrupts for all devices that cause interrupts, and enable
	 * the INTC master enable bit.
	 */
//	XIntc_EnableIntr(XPAR_INTC_0_BASEADDR, XIN_INT_MASTER_ENABLE_MASK);

    functions_tbl[0] = action1;
    functions_tbl[1] = action2;
    functions_tbl[2] = action3;

//	init_lrt(DOMAIN,XPAR_CPU_ID,0,0,0);
    init_lrt(XPAR_MAILBOX_0_IF_1_DEVICE_ID, XPAR_MAILBOX_0_IF_1_BASEADDR);

    cleanup_platform();

    return 0;
}
