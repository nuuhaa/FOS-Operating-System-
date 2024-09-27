#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>
bool BSD=0;

uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}
uint32 one_second_reseting_tick;//a tick counter that resets every one second
uint32 four_second_reseting_tick;
//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(1);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP
	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	BSD=1;
	sched_delete_ready_queues();
	one_second_reseting_tick=0;
	four_second_reseting_tick=0;
	load_avg=fix_int(0);
	num_of_ready_queues=numOfLevels;
	env_ready_queues=(struct Env_Queue*)kmalloc(numOfLevels*sizeof(struct Env_Queue));
	quantums=(uint8*)kmalloc(sizeof(uint8));
	quantums[0]=quantum;
	kclock_set_quantum(quantums[0]);
	for (int var = 0; var < numOfLevels; var++) {
		init_queue(&(env_ready_queues[var]));
	}
	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}
bool n_seconds_passed(int n,uint32 ticker){
	if(ticker*quantums[0]>=n*1000)//1000 because quantum is in milliseconds...
		return 1;
	else
		return 0;

}

//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================

struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");


		if (curenv != NULL)
				{	//cprintf("ID:%d\t Nice:%d\t Priority:%d\n",curenv->env_id,curenv->nice,curenv->priority);
					struct Env* iterator=LIST_LAST(&(env_ready_queues[curenv->priority]));
					bool inserted=0;
					//while(iterator!=NULL){
						///if(curenv->nice<iterator->nice){
						///	LIST_INSERT_AFTER(&(env_ready_queues[curenv->priority]),iterator,curenv);
						//	inserted=1;
					//		break;
					///	}
						//iterator=LIST_PREV(iterator);
					//}

					if(!inserted)
						enqueue(&(env_ready_queues[curenv->priority]),curenv);
				}
				struct Env *next_env = NULL;
				int n=num_of_ready_queues-1;
				while(n>=0){
					int size=queue_size(&(env_ready_queues[n]));
					if(size>0){
						next_env=dequeue(&(env_ready_queues[n]));
						break;
					}
					n--;
				}

				kclock_set_quantum(quantums[0]);
				if(next_env==NULL){
					cprintf("didn't find any next env...\n");
					load_avg=fix_int(0);
				}

			return next_env;

}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
int num_ready_processes(){
	int processes=0;
	if(curenv!=NULL)
		processes+=1;
	for (int var = 0; var < num_of_ready_queues; var++) {
		processes+=queue_size(&(env_ready_queues[var]));
	}
	return processes;
}

void update_load_avg(){
	//Equation : (59/60)*load_avg(t-1)+(1/60)*ready_processes
	fixed_point_t numerator=fix_int(59);
	fixed_point_t denominator=fix_int(60);
	fixed_point_t fraction=fix_div(numerator,denominator);
	fixed_point_t first_operand=fix_mul(fraction,load_avg);
	numerator=fix_int(1);
	denominator=fix_int(60);
	fraction=fix_div(numerator,denominator);
	fixed_point_t second_operand=fix_mul(fraction,fix_int(num_ready_processes()));
	fixed_point_t result=fix_add(first_operand,second_operand);
	load_avg=result;
	//should be updated every one second...
}
void update_env_recent_cpu(struct Env* env){
	//Equation:(2*load_avg/(2*load_avg+1))*recent_cpu(t-1)+nice
	fixed_point_t numerator=fix_mul(fix_int(2),load_avg);
	fixed_point_t denominator=fix_add(numerator,fix_int(1));
	fixed_point_t fraction=fix_div(numerator,denominator);
	fixed_point_t first_operand=fix_mul(fraction,env->recent_cpu);
	fixed_point_t result=fix_add(first_operand,fix_int(env->nice));
	env->recent_cpu=result;
	//cprintf("Env ID:%d\tRecent Load Average:%d\n",env->env_id,fix_round(load_avg));
	//cprintf("Env ID: %d\t CPU Usage:%d\n",env->env_id,fix_round(env->recent_cpu));
	//should be updated every one second for each env in ready queue
}
void test_arithmetic(){
	//Eq:2.5*7/10
	//just a test function don't give too much attention to it...
	fixed_point_t first_operand=fix_div(fix_int(5),fix_int(2));
	fixed_point_t second_operand=fix_div(fix_int(7),fix_int(10));
	fixed_point_t result = fix_mul(first_operand,second_operand);
	int integer_result=fix_round(result);
	cprintf("result :%d\n",integer_result);
}
void update_all_recent_cpu(){
	update_env_recent_cpu(curenv);
	for (int var = 0; var < num_of_ready_queues; var++) {
		struct Env* temp_env=NULL;
		LIST_FOREACH(temp_env,&(env_ready_queues[var])){
			update_env_recent_cpu(temp_env);

		}
	}
}
void update_env_priority(struct Env* env){
	//Equation: PRI_MAX-(recent_cpu/4)-(nice*2)
	fixed_point_t first_operand=fix_int(PRI_MAX);
	fixed_point_t second_operand=fix_unscale(env->recent_cpu,4);
	fixed_point_t third_operand=fix_scale(fix_int(env->nice),2);
	fixed_point_t result=fix_sub(first_operand,second_operand);
	result=fix_sub(result,third_operand);
	//cprintf("Env ID:%d \t Result:%d\n",env->env_id,fix_round(result));
	int integer_result=fix_trunc(result);
	if(integer_result>num_of_ready_queues-1)
		integer_result=num_of_ready_queues-1;
	if(integer_result<0)
		integer_result=0;
	env->priority=integer_result;
}
void update_all_priorities(){
	//cprintf("\n=================================\n");
	int updated_env=0;
	//cprintf("Current Env:");
	update_env_priority(curenv);
	updated_env++;
	for (int var = num_of_ready_queues-1; var >= 0; --var) {
		struct Env*iterator=LIST_LAST(&(env_ready_queues[var]));
		while(iterator!=NULL){
			update_env_priority(iterator);
				updated_env++;
			iterator=LIST_PREV(iterator);
		}

	}
	//cprintf("#updated envs:%d\n",updated_env);
	//cprintf("==================================\n");
}




void organize_all_queues(){
	for (int var = num_of_ready_queues-1; var >= 0; --var) {
		struct Env* iterator=LIST_LAST(&(env_ready_queues[var]));
		while(iterator!=NULL){
			bool preved=0;
			if(iterator->priority!=var){
				struct Env* removed_env=iterator;
				iterator=LIST_PREV(iterator);
				preved=1;
				remove_from_queue(&(env_ready_queues[var]),removed_env);
				struct Env*inner_iterator=LIST_LAST(&(env_ready_queues[removed_env->priority]));
				bool inserted=0;
				while(inner_iterator!=NULL){
					if(removed_env->nice<inner_iterator->nice){
						LIST_INSERT_AFTER(&(env_ready_queues[removed_env->priority]),inner_iterator,removed_env);
						inserted=1;
					//	cprintf("insert Condition reached!\n");
						break;
					}
					inner_iterator=LIST_PREV(inner_iterator);
				}
				if(!inserted)
					enqueue(&(env_ready_queues[removed_env->priority]),removed_env);

			}
			if(!preved)
		iterator=LIST_PREV(iterator);
	}
}
}
void show_all_priorities(){

	cprintf("Current Env ID:%d\tCurrent Env Priority: %d\n",curenv->env_id,curenv->priority);
	for (int var = 0; var < num_of_ready_queues; ++var) {
		struct Env* env;
		LIST_FOREACH(env,&(env_ready_queues[var])){
			cprintf("Env ID:%d\t Env Priority:%d\n",env->env_id,env->priority);
		}
	}
}
void show_queue(int level){
	cprintf("\n--------------------------------\n");
	cprintf("Queue Level :%d\n",level);
	struct Env* iterator=LIST_LAST(&(env_ready_queues[level]));
	while(iterator!=NULL){
		cprintf("Env ID:%d\n",iterator->env_id);
		iterator=LIST_PREV(iterator);
	}
	cprintf("\n--------------------------------\n");
}
void show_all_nonEmpty_queues(){
	for (int var = num_of_ready_queues-1; var >= 0; --var) {
		if(queue_size(&(env_ready_queues[var]))!=0)
		show_queue(var);
	}
}

void increase_env_recent_cpu(struct Env* env){
	fixed_point_t first_operand=fix_int(1);
	fixed_point_t result=fix_add(env->recent_cpu,first_operand);
	env->recent_cpu=result;
}
void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	{	if(BSD){
		one_second_reseting_tick++;

			if(n_seconds_passed(1,one_second_reseting_tick)){
				//update load avg for kernel
				update_load_avg();
				//update all recent cpu for all envs...
				update_all_recent_cpu();
				//reset the ticker
				//show_all_priorities();
				one_second_reseting_tick=0;
			}
			if(ticks%4==0){
				//update all priorities
				update_all_priorities();

				//reset the ticker
				organize_all_queues();
				//show_all_nonEmpty_queues();

			}
			//increase recent cpu every tick for the running env
			increase_env_recent_cpu(curenv);
	}

	}

	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;

	//cprintf("one second ticker:%lu \t four second ticker:%lu\n",one_second_reseting_tick,four_second_reseting_tick);

	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}

