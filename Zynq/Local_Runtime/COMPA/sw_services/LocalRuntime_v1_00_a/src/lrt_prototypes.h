/********************************************************************************
 * File    : lrt_prototypes.h
 * By      : Yaset Oliva Venegas
 * Version : 1.0
 * Date	   : October 2012
 * Descrip : This file contains the prototypes of the RunTime's functions.
 * 			Prototypes should be MTAPI-compliant.
*********************************************************************************/

#ifndef RT_PROTOTYPES
#define RT_PROTOTYPES

//#include "mtapi/mtapi.h"
#include "lrt_definitions.h"





///* ************** At lrt_initialization.h ****************
// *
// * Function : mtapi_initialization
// * Params   :
// * Return   :
// * Descrip  : Initializes the MTAPI.
//*/
//extern void mtapi_initialization();








/***************** At lrt_core.c *****************************
*********************************************************************************************************
*                                            OSStartCur
*
* Description: Starts the task pointed by the OSTCBCur.
*
*********************************************************************************************************
*/

extern void OSStartCur();



/*
 * Descrip	: Iterates on the OSTCBList until there is no more active tasks.
 */
extern void OS_Sched();








/* ************** At lrt_task_mngr.c ****************
 *
 * Function : mtapi_task_create
 * Params   : funct_addr	is pointer to the functional code.
 * Return   :
 * Descrip  : Creates an task.
*/
extern void mtapi_task_create(void* funct_addr);







/*
 * Function : mtapi_task_start
 * Params   : 
			  funct_addr	is the address of the functional code of the targeted task.
 * Descrip  : Starts a task.
*/


/*
*********************************************************************************************************
*                                     CREATE A TASK (Extended Version)
*
* Description: This function is used to have uC/OS-II manage the execution of a task.  Tasks can either
*              be created prior to the start of multitasking or by a running task.  A task cannot be
*              created by an ISR.  This function is similar to OSTaskCreate() except that it allows
*              additional information about a task to be specified.
*
* Arguments  : task      is a pointer to the task's code
*
*              p_arg     is a pointer to an optional data area which can be used to pass parameters to
*                        the task when the task first executes.  Where the task is concerned it thinks
*                        it was invoked and passed the argument 'p_arg' as follows:
*
*                            void Task (void *p_arg)
*                            {
*                                for (;;) {
*                                    Task code;
*                                }
*                            }
*
*              ptos      is a pointer to the task's top of stack.  If the configuration constant
*                        OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                        memory to low memory).  'ptos' will thus point to the highest (valid) memory
*                        location of the stack.  If OS_STK_GROWTH is set to 0, 'ptos' will point to the
*                        lowest memory location of the stack and the stack will grow with increasing
*                        memory locations.  'ptos' MUST point to a valid 'free' data item.
*
*              prio      is the task's priority.  A unique priority MUST be assigned to each task and the
*                        lower the number, the higher the priority.
*
*              id        is the task's ID (0..65535)
*
*              pbos      is a pointer to the task's bottom of stack.  If the configuration constant
*                        OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                        memory to low memory).  'pbos' will thus point to the LOWEST (valid) memory
*                        location of the stack.  If OS_STK_GROWTH is set to 0, 'pbos' will point to the
*                        HIGHEST memory location of the stack and the stack will grow with increasing
*                        memory locations.  'pbos' MUST point to a valid 'free' data item.
*
*              stk_size  is the size of the stack in number of elements.  If OS_STK is set to INT8U,
*                        'stk_size' corresponds to the number of bytes available.  If OS_STK is set to
*                        INT16U, 'stk_size' contains the number of 16-bit entries available.  Finally, if
*                        OS_STK is set to INT32U, 'stk_size' contains the number of 32-bit entries
*                        available on the stack.
*
*			   nb_fifo_in 	is the number of elements in the fifo_in array.
*
*			   nb_fifo_out 	is the number of elements in the fifo_out array.
*
*              fifo_in 	 is the array of identifiers of the input FIFOs.
*
*              fifo_out  is the array of identifiers of the input FIFOs.
*
* 			   nb_args	 is the number of elements of the args array.
*
*              args      is the array of arguments to be passed to the function.
*
*              opt       contains additional information (or options) about the behavior of the task.  The
*                        LOWER 8-bits are reserved by uC/OS-II while the upper 8 bits can be application
*                        specific.  See OS_TASK_OPT_??? in uCOS-II.H.  Current choices are:
*
*                        OS_TASK_OPT_STK_CHK      Stack checking to be allowed for the task
*                        OS_TASK_OPT_STK_CLR      Clear the stack when the task is created
*                        OS_TASK_OPT_SAVE_FP      If the CPU has floating-point registers, save them
*                                                 during a context switch.
*
* Returns    : OS_ERR_NONE             if the function was successful.
*              OS_PRIO_EXIT            if the task priority already exist
*                                      (each task MUST have a unique priority).
*              OS_ERR_PRIO_INVALID     if the priority you specify is higher that the maximum allowed
*                                      (i.e. > OS_LOWEST_PRIO)
*              OS_ERR_TASK_CREATE_ISR  if you tried to create a task from an ISR.
*********************************************************************************************************
*/

extern INT8U  OSTaskCreateExt (FUNCTION_TYPE task,
								void    *p_arg,
								OS_STK  *ptos,
								INT8U    prio,
								INT16U   id,
								OS_STK  *pbos,
								INT32U   stk_size,
								INT16U	nb_fifo_in,
								INT16U	nb_fifo_out,
								INT32U	*fifo_in,
								INT32U	*fifo_out,
								INT32U	nb_args,
								INT32U  *args,
								INT16U  opt);











/*
*********************************************************************************************************
*                                            QUERY A TASK
*
* Description: This function is called to obtain a copy of the desired task's TCB.
*
* Arguments  : id         is the id. of the task to obtain information from.
*
*              p_task_data  is a pointer to where the desired task's OS_TCB will be stored.
*
* Returns    : OS_ERR_NONE            if the requested task is suspended
*              OS_ERR_PRIO_INVALID    if the priority you specify is higher that the maximum allowed
*                                     (i.e. > OS_LOWEST_PRIO) or, you have not specified OS_PRIO_SELF.
*              OS_ERR_PRIO            if the desired task has not been created
*              OS_ERR_TASK_NOT_EXIST  if the task is assigned to a Mutex PIP
*              OS_ERR_PDATA_NULL      if 'p_task_data' is a NULL pointer
*********************************************************************************************************
*/

extern INT8U  OSTaskQuery (INT8U id, OS_TCB  *p_task_data);













/*
*********************************************************************************************************
*                                            DELETE A TASK
*
* Description: This function allows you to delete a task from the list of available tasks...
*
* Arguments  : id    is the identifier of the task to delete.
* 			   curr_vertex_id is will contain the id. of the current vertex of the deleted task.
*
* Returns    : OS_ERR_NONE             if the call is successful
*              OS_ERR_TASK_NOT_EXIST   if the task you want to delete does not exist.
*
*********************************************************************************************************
*/

extern INT8U  OSTaskDel (INT8U id, INT32U *curr_vertex_id);








/*
*********************************************************************************************************
*                                     CREATE A LRT's TASK
*
* Description: Creates a Lrt's task.
*
* Arguments  : ptos      is a pointer to the task's top of stack.  If the configuration constant
*                        OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                        memory to low memory).  'ptos' will thus point to the highest (valid) memory
*                        location of the stack.  If OS_STK_GROWTH is set to 0, 'ptos' will point to the
*                        lowest memory location of the stack and the stack will grow with increasing
*                        memory locations.  'ptos' MUST point to a valid 'free' data item.
*
*              prio      is the task's priority.  A unique priority MUST be assigned to each task and the
*                        lower the number, the higher the priority.
*
*              id        is the task's ID (0..65535)
*
*              pbos      is a pointer to the task's bottom of stack.  If the configuration constant
*                        OS_STK_GROWTH is set to 1, the stack is assumed to grow downward (i.e. from high
*                        memory to low memory).  'pbos' will thus point to the LOWEST (valid) memory
*                        location of the stack.  If OS_STK_GROWTH is set to 0, 'pbos' will point to the
*                        HIGHEST memory location of the stack and the stack will grow with increasing
*                        memory locations.  'pbos' MUST point to a valid 'free' data item.
*
*              stk_size  is the size of the stack in number of elements.  If OS_STK is set to INT8U,
*                        'stk_size' corresponds to the number of bytes available.  If OS_STK is set to
*                        INT16U, 'stk_size' contains the number of 16-bit entries available.  Finally, if
*                        OS_STK is set to INT32U, 'stk_size' contains the number of 32-bit entries
*                        available on the stack.
*
*			   nb_fifo_in 		is the number of elements in the fifo_in array.
*
*			   nb_fifo_out 		is the number of elements in the fifo_out array.
*
*              fifo_in 	 		is the array of identifiers of the input FIFOs.
*
*              fifo_out  		is the array of identifiers of the input FIFOs.

* 			   nb_am_vertices	is the number of vertices in the actor machine.
*
*              am_vertices		is the array of actor machine's vertices.
*
*              opt       contains additional information (or options) about the behavior of the task.  The
*                        LOWER 8-bits are reserved by uC/OS-II while the upper 8 bits can be application
*                        specific.  See OS_TASK_OPT_??? in uCOS-II.H.  Current choices are:
*
*                        OS_TASK_OPT_STK_CHK      Stack checking to be allowed for the task
*                        OS_TASK_OPT_STK_CLR      Clear the stack when the task is created
*                        OS_TASK_OPT_SAVE_FP      If the CPU has floating-point registers, save them
*                                                 during a context switch.

*
* Returns    : OS_ERR_NONE             if the function was successful.
*              OS_PRIO_EXIT            if the task priority already exist
*                                      (each task MUST have a unique priority).
*              OS_ERR_PRIO_INVALID     if the priority you specify is higher that the maximum allowed
*                                      (i.e. > OS_LOWEST_PRIO)
*              OS_ERR_TASK_CREATE_ISR  if you tried to create a task from an ISR.
*********************************************************************************************************
*/
extern INT8U  LrtTaskCreate 	(OS_STK  				*ptos,
								OS_STK  				*pbos,
								INT32U  				stk_size,
								MSG_CREATE_TASK_STRUCT	*msg_create_task);
//extern INT8U  LrtTaskCreate 	(OS_STK  				*ptos,
//								INT8U    				prio,
//								INT16U   				id,
//								OS_STK  				*pbos,
//								INT32U  				stk_size,
//								INT16U					nb_fifo_in,
//								INT16U					nb_fifo_out,
//								INT32U					*fifo_in,
//								INT32U					*fifo_out,
//								INT32U					nb_am_vertices,
//								AM_VERTEX_STRUCT		*am_vertices,
//		                        INT32U					nb_am_conditions,
//		                        AM_ACTOR_COND_STRUCT	*am_conditions,
//								INT16U  				opt);



/*
*********************************************************************************************************
*                                              wait_for_ext_typed_msg
*
* Description: Waits for an external message is placed into the mailbox.
*
* Arguments  : addr is the base address of the mailbox.
*
* Returns    : none
*
*********************************************************************************************************
*/
extern void  wait_ext_msg();



//
///*
//*********************************************************************************************************
//*                                              get_ext_msg
//*
//* Description: Checks the mailbox for new messages.
//*
//* Arguments  :
//*
//* Returns    : none
//*
//*********************************************************************************************************
//*/
//extern void  get_ext_msg();
//




///*$PAGE*/
///*
//*********************************************************************************************************
//*                                              read_fsl_fifo (work in progress!!)
//*
//* Description: It reads data from the FSL fifo.
//*
//* Arguments  :
//*
//* Returns    : none
//*
//*********************************************************************************************************
//*/
//extern void  read_fsl_fifo();







///*$PAGE*/
///*
//*********************************************************************************************************
//*                                              write_fsl_fifo (work in progress!!)
//*
//* Description: It writes data into the FSL fifo.
//*
//* Arguments  :
//*
//* Returns    : none
//*
//*********************************************************************************************************
//*/
//extern void  write_fsl_fifo(INT32U);











/*
*********************************************************************************************************
*                                              create_fifo_hndl
*
* Description: Creates a handle for a FIFO.
*
* Arguments  : msg_create_fifo is a pointer to the received data.
* 			   perr will contain one of these error codes : OS_ERROR_NONE, OS_ERR_FIFO_INVALID_ID.
*
* Returns    : a pointer to a FIFO handle.
*
*********************************************************************************************************
*/
extern LRT_FIFO_HNDLE* create_fifo_hndl(MSG_CREATE_FIFO_STRUCT *msg_create_fifo, INT8U* perr);









/*
*********************************************************************************************************
*                                              clear_fifo
*
* Description: Clears the contents of a FIFO by reinitializing its indices .
*
* Arguments  : id is the index of the FIFO.
* 			   perr will contain one of these error codes : OS_ERROR_NONE, OS_ERR_FIFO_INVALID_ID.
*
* Returns    :
*
*********************************************************************************************************
*/
extern void clear_fifo(INT16U id, INT8U* perr);






/*
*********************************************************************************************************
*                                              get_fifo_hndl
*
* Description: Gets the pointer to a FIFO handle.
*
* Arguments  : fifo_id
* 			   perr will contain the error code : OS_ERR_NONE or OS_ERR_FIFO_INVALID_ID
*
*
* Returns    : A pointer to the FIFO or NULL if it doesn't found it.
*
*********************************************************************************************************
*/
extern LRT_FIFO_HNDLE* get_fifo_hndl(INT16U fifo_id, INT8U* perr);





/*
*********************************************************************************************************
*                                              check_input_fifo
*
* Description: Checks whether a data block can be read from an input FIFO.
*
* Arguments  : in_fifo_hndl is a pointer to the input FIFO's handle.
* 			   size is the amount of data to be read in bytes.
*
* Returns	 : true if there is enough data.
*
*********************************************************************************************************
*/
extern BOOLEAN  check_input_fifo(LRT_FIFO_HNDLE* in_fifo_hndl, INT32U size);







/*
*********************************************************************************************************
*                                              read_input_fifo
*
* Description: Reads data (tokens) from an input FIFO.
*
* Arguments  : in_fifo_hndl is a pointer to the input FIFO's handle.
* 			   size is the amount of data to be read in bytes.
* 			   buffer is a pointer to a data block that will store the read data.
*			   perr will contain the error code : OS_ERR_NONE or OS_ERR_FIFO_NO_ENOUGH_DATA
*
*********************************************************************************************************
*/
extern void  read_input_fifo(LRT_FIFO_HNDLE* in_fifo_hndl, INT32U size, INT8U* buffer, INT8U *perr);










/*
*********************************************************************************************************
*                                              check_output_fifo
*
* Description: Checks whether a data block can be written into an output FIFO.
*
* Arguments  : out_fifo_hndl is a pointer to output FIFO's handle.
* 			   size is the amount of data to be written in bytes.
*
* Returns    : true if there is enough space in the FIFO.
*
*********************************************************************************************************
*/
extern BOOLEAN  check_output_fifo(LRT_FIFO_HNDLE* out_fifo_hndl, INT32U size);









/*
*********************************************************************************************************
*                                              write_output_fifo
*
* Description: Writes data (tokens) into an output FIFO.
*
* Arguments  : out_fifo_hndl is a pointer to output FIFO's handle.
* 			   size is the amount of data to be written in bytes.
* 			   buffer is a pointer to the data block to be copied.
* 			   perr will contain the error code : OS_ERR_NONE or OS_ERR_FIFO_NO_ENOUGH_ESPACE.
*
* Returns    :
*
*********************************************************************************************************
*/
extern void  write_output_fifo(LRT_FIFO_HNDLE* out_fifo_hndl, INT32U size, INT8U* buffer, INT8U *perr);











/*$PAGE*/
/*
*********************************************************************************************************
*                                              blocking_read_input_fifo
*
* Description: Reads data (tokens) from an input FIFO. Blocks until there is enough data.
*
* Arguments  : in_fifo_hndl is a pointer to the input FIFO's handle.
* 			   size is the amount of data to be read in bytes.
* 			   buffer is a pointer to a data block that will store the read data.
* 			   perr will contain the error code : OS_ERR_NONE.
*
* Returns    :
*
*********************************************************************************************************
*/
extern void  blocking_read_input_fifo(LRT_FIFO_HNDLE* in_fifo_hndl, INT32U size, INT8U* buffer, INT8U *perr);














/*
*********************************************************************************************************
*                                              blocking_write_output_fifo
*
* Description: Writes data (tokens) to an output FIFO. Blocks until there is enough space.
*
* Arguments  : out_fifo_hndl is a pointer to the output FIFO's handle.
* 			   size is the amount of data to be written in bytes.
* 			   buffer is a pointer to a data block containing the data.
* 			   perr will contain the error code : OS_ERR_NONE.
*
* Returns    :
*
*********************************************************************************************************
*/
extern void  blocking_write_output_fifo(LRT_FIFO_HNDLE* out_fifo_hndl, INT32U size, INT8U* buffer, INT8U *perr);


















/* ************** At lrt_core.c file****************
 *
 */

/*
*********************************************************************************************************
*                                        COPY A BLOCK OF MEMORY
*
* Description: This function is called by other uC/OS-II services to copy a block of memory from one
*              location to another.
*
* Arguments  : pdest    is a pointer to the 'destination' memory block
*
*              psrc     is a pointer to the 'source'      memory block
*
*              size     is the number of bytes to copy.
*
* Returns    : none
*
* Notes      : 1) This function is INTERNAL to uC/OS-II and your application should not call it.  There is
*                 no provision to handle overlapping memory copy.  However, that's not a problem since this
*                 is not a situation that will happen.
*              2) Note that we can only copy up to 64K bytes of RAM
*              3) The copy is done one byte at a time since this will work on any processor irrespective
*                 of the alignment of the source and destination.
*********************************************************************************************************
*/

extern void  OS_MemCopy (INT8U  *pdest, INT8U  *psrc, INT16U  size);

















/* ************** At lrt_debug.c file****************
 *
 */

extern void print(char *ptr);


extern void putnum(unsigned int num);

extern void putnum_dec(unsigned int num);

#endif