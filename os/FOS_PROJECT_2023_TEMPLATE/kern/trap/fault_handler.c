/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================
struct WorkingSetElement* check_position_of_VA(struct Env* curenv,uint32 fault_va)
{
	struct WorkingSetElement* curr = NULL;
	LIST_FOREACH(curr,&(curenv->SecondList))
	{
		if((curr->virtual_address& (uint32)0xFFFFF000 )== (fault_va& (uint32)0xFFFFF000))
			return curr;
	}
	return NULL;
}

void get_frame_and_map_it_for_ws(struct Env* curenv,uint32 fault_va)
{
	struct FrameInfo* frame = NULL;
	allocate_frame(&frame);
	map_frame(curenv->env_page_directory,frame,fault_va,PERM_WRITEABLE|PERM_USER);
	struct WorkingSetElement* elm = env_page_ws_list_create_element(curenv,fault_va);
	LIST_INSERT_HEAD(&(curenv->ActiveList),elm);
}
void check_permission(struct Env* curenv,uint32 fault_va)
{
	uint32 page_permissions = pt_get_page_permissions(curenv->env_page_directory, fault_va);
	if(page_permissions & PERM_MODIFIED)
	{
		uint32* ptr_page_table;
		struct FrameInfo* frame = get_frame_info(curenv->env_page_directory,fault_va,&ptr_page_table);
		pf_update_env_page(curenv,fault_va,frame);
	}
}
struct WorkingSetElement* make_frame(struct Env* curenv,uint32 fault_va)
{
	uint32* ptr_page_table;
	struct FrameInfo* frame = NULL;
	allocate_frame(&frame);
	map_frame(curenv->env_page_directory,frame,fault_va,PERM_WRITEABLE|PERM_USER);
	pf_read_env_page(curenv,(void*)fault_va);
	struct WorkingSetElement* elm = env_page_ws_list_create_element(curenv,fault_va);
	return elm;
}
void va_placment(struct Env* curenv,uint32 fault_va)
{
	struct WorkingSetElement* elm = make_frame(curenv,fault_va);
	if(pf_read_env_page(curenv,(void*)fault_va)==E_PAGE_NOT_EXIST_IN_PF){
		bool do_not_kill=0;
		if(fault_va>=USTACKBOTTOM && fault_va<=USTACKTOP)
		{
			do_not_kill=1;
		}
		if(fault_va>=USER_HEAP_START && fault_va<=USER_HEAP_MAX)
		{
			do_not_kill=1;
		}
		if(!do_not_kill){
			//curenv->pageFaultsCounter++;
			cprintf("killed in va placement\n");
			sched_kill_env(curenv->env_id);
			return;
		}
	}
	LIST_INSERT_HEAD(&(curenv->ActiveList),elm);
}
//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif

		//refer to the project presentation and documentation for details

		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		if(isPageReplacmentAlgorithmFIFO())
		{
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
			struct WorkingSetElement *victimWSElement = NULL;
			uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
			if(wsSize < (curenv->page_WS_max_size))
			{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
				//cprintf("placement has been called!\n");
			struct WorkingSetElement* element = make_frame(curenv,fault_va);
			if(pf_read_env_page(curenv,(void*)fault_va)==E_PAGE_NOT_EXIST_IN_PF){
				bool do_not_kill=0;
				if(fault_va>=USTACKBOTTOM && fault_va<=USTACKTOP)
					do_not_kill=1;
				if(fault_va>=USER_HEAP_START && fault_va<=USER_HEAP_MAX)
					do_not_kill=1;
				if(!do_not_kill){
					//cprintf("killed in placement...1\n");
					//cprintf("faulted va : %x\n",fault_va);
					curenv->pageFaultsCounter++;
					sched_kill_env(curenv->env_id);
					return ;

				}
			}
			LIST_INSERT_TAIL(&curenv->page_WS_list,element);
			if(LIST_SIZE(&curenv->page_WS_list)<curenv->page_WS_max_size)
				curenv->page_last_WS_element=NULL;
			else {
				curenv->page_last_WS_element=LIST_FIRST(&curenv->page_WS_list);
			}
		}
		else
		{
			if(curenv->page_last_WS_element == LIST_LAST(&curenv->page_WS_list))
			{
				struct WorkingSetElement* elm = make_frame(curenv,fault_va);
				if(pf_read_env_page(curenv,(void*)fault_va)==E_PAGE_NOT_EXIST_IN_PF){
					bool do_not_kill=0;
					if(fault_va>=USTACKBOTTOM && fault_va<=USTACKTOP)
						do_not_kill=1;
					if(fault_va>=USER_HEAP_START && fault_va<=USER_HEAP_MAX)
						do_not_kill=1;
					if(!do_not_kill){
						//cprintf("killed in placement... 2\n");
						curenv->pageFaultsCounter++;
						sched_kill_env(curenv->env_id);
						return;
					}
				}
				curenv->page_last_WS_element = LIST_FIRST(&curenv->page_WS_list);
				struct WorkingSetElement *temp = LIST_LAST(&curenv->page_WS_list);
				check_permission(curenv,temp->virtual_address);
				unmap_frame(curenv->env_page_directory,temp->virtual_address);
				LIST_REMOVE(&(curenv->page_WS_list),temp);
				LIST_INSERT_TAIL(&(curenv->page_WS_list),elm);
			}
			else
			{
				struct WorkingSetElement* elm = make_frame(curenv,fault_va);
				if(pf_read_env_page(curenv,(void*)fault_va)==E_PAGE_NOT_EXIST_IN_PF){
					bool do_not_kill=0;
					if(fault_va>=USTACKBOTTOM && fault_va<=USTACKTOP)
						do_not_kill=1;

					if(fault_va>=USER_HEAP_START && fault_va<=USER_HEAP_MAX)
						do_not_kill=1;
					if(!do_not_kill){
						//cprintf("killed in placement...\n");
						curenv->pageFaultsCounter++;
						sched_kill_env(curenv->env_id);
						return;
					}
				}
				curenv->page_last_WS_element = LIST_NEXT(curenv->page_last_WS_element);
				struct WorkingSetElement *temp = LIST_PREV(curenv->page_last_WS_element);
				check_permission(curenv,temp->virtual_address);
				unmap_frame(curenv->env_page_directory,temp->virtual_address);
				LIST_REMOVE(&(curenv->page_WS_list),temp);
				LIST_INSERT_BEFORE(&(curenv->page_WS_list),curenv->page_last_WS_element,elm);
			}
		}
	}


		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
			if(LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)) < curenv->page_WS_max_size)
			{
				int ActiveSize = LIST_SIZE(&(curenv->ActiveList));
				int SCSize = LIST_SIZE(&(curenv->SecondList));
				if(ActiveSize < curenv->ActiveListSize &&SCSize ==0)
				{
					va_placment(curenv,fault_va);
				}
				else if(ActiveSize <= curenv->ActiveListSize&& SCSize < curenv->SecondListSize)
				{
				  if(ActiveSize ==curenv->ActiveListSize)
				  {
					 struct WorkingSetElement* ret = check_position_of_VA(curenv,fault_va);
					 if(ret ==NULL)
					 {
					   struct WorkingSetElement* last_active_first_sc = LIST_LAST(&(curenv->ActiveList));
					   pt_set_page_permissions(curenv->env_page_directory,last_active_first_sc->virtual_address,0,PERM_PRESENT);
				   	   LIST_REMOVE(&(curenv->ActiveList),last_active_first_sc);
					   LIST_INSERT_HEAD(&(curenv->SecondList),last_active_first_sc);
				   	   va_placment(curenv,fault_va);
					 }
					 else
					 {
						 struct WorkingSetElement* last_active_first_sc = LIST_LAST(&(curenv->ActiveList));
						 pt_set_page_permissions(curenv->env_page_directory,last_active_first_sc->virtual_address,0,PERM_PRESENT);
					   	 LIST_REMOVE(&(curenv->ActiveList),last_active_first_sc);
						 LIST_INSERT_HEAD(&(curenv->SecondList),last_active_first_sc);
						 pt_set_page_permissions(curenv->env_page_directory,ret->virtual_address,PERM_PRESENT,0);
					   	 LIST_REMOVE(&(curenv->SecondList),ret);
						 LIST_INSERT_HEAD(&(curenv->ActiveList),ret);
					 }
				  }
				  else
				  {
					struct WorkingSetElement* ret = check_position_of_VA(curenv,fault_va);
					if(ret ==NULL)
					{
						va_placment(curenv,fault_va);
					}
					else
					{
						pt_set_page_permissions(curenv->env_page_directory,ret->virtual_address,PERM_PRESENT,0);
						LIST_INSERT_HEAD(&(curenv->ActiveList),ret);
					    LIST_REMOVE(&(curenv->SecondList),ret);
					}
				  }
				}
			}
			else
			{
				struct WorkingSetElement* ret = check_position_of_VA(curenv,fault_va& 0xFFFFF000);
				if(ret ==NULL)
				{
					struct WorkingSetElement* last_sc = LIST_LAST(&curenv->SecondList);
					check_permission(curenv,last_sc->virtual_address);
					unmap_frame(curenv->env_page_directory,last_sc->virtual_address);
					LIST_REMOVE(&curenv->SecondList,last_sc);
					struct WorkingSetElement* last_active = LIST_LAST(&curenv->ActiveList);
					pt_set_page_permissions(curenv->env_page_directory,last_active->virtual_address,0,PERM_PRESENT);
					LIST_REMOVE(&curenv->ActiveList,last_active);
					LIST_INSERT_HEAD(&curenv->SecondList,last_active);
					struct WorkingSetElement* elm = make_frame(curenv,fault_va);
					LIST_INSERT_HEAD(&(curenv->ActiveList),elm);

				}
				else
				{
					pt_set_page_permissions(curenv->env_page_directory,ret->virtual_address,PERM_PRESENT,0);
					LIST_REMOVE(&curenv->SecondList,ret);
					struct WorkingSetElement* last_active = LIST_LAST(&curenv->ActiveList);
					pt_set_page_permissions(curenv->env_page_directory,last_active->virtual_address,0,PERM_PRESENT);
					LIST_REMOVE(&curenv->ActiveList,last_active);
					LIST_INSERT_HEAD(&curenv->SecondList,last_active);
					LIST_INSERT_HEAD(&curenv->ActiveList,ret);

				}

			}
			//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}

}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}



