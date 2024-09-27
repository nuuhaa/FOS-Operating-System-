#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
LIST_HEAD(page_list, page_block);
struct page_list allocator_list;
struct page_block
{
	uint32 lower_bound;
	uint32 upper_bound;
	LIST_ENTRY(page_block) prev_next_info;	/* linked list links */
};

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================

#define PAGE_ALLOC_START USER_HEAP_START+DYN_ALLOC_MAX_SIZE+PAGE_SIZE
#define PAGE_ALLOC_END USER_HEAP_MAX
uint32 needed_pages_for_size(uint32 size){
	uint32 pages=0;

	if(size<=PAGE_SIZE && size!=0){
		pages=1;
		return pages;
	}
	else if(size==0)
	{
		cprintf("zero size!!!\n");
		return 0;
	}
	else if(size>PAGE_SIZE){
		pages+=(size/PAGE_SIZE);
		uint32 temp=size%PAGE_SIZE;
		if(temp!=0){
			pages+=1;
		}
		return pages;
	}
	return pages;
}
bool first_time=1;
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");
	//return NULL;
	//cprintf("user malloc has been called!\n");
	if(first_time){
		LIST_INIT(&allocator_list);
		first_time=0;
	}

		//sbrk(-20);
	//cprintf("test  get hard limit : %x        %x\n",sys_get_hard_limit(),USER_HEAP_MAX+PAGE_SIZE+DYN_ALLOC_MAX_SIZE);

	if(size<=DYN_ALLOC_MAX_BLOCK_SIZE && sys_isUHeapPlacementStrategyFIRSTFIT()){
		user_list_constant=1;

		return alloc_block_FF(size);
	}
	else if(size>DYN_ALLOC_MAX_BLOCK_SIZE && sys_isUHeapPlacementStrategyFIRSTFIT()){
		uint32 pages_needed=needed_pages_for_size(size);
		uint32 first_fit=sys_first_fit(pages_needed);
		if(first_fit==0)
			return NULL;
		sys_allocate_user_mem(first_fit,pages_needed*PAGE_SIZE);
		//cprintf("returned address :%x\n",first_fit);
		user_list_constant=1;
		struct page_block *block=(struct page_block*)alloc_block_FF(sizeof(struct page_block));
		block->lower_bound=first_fit;
		block->upper_bound=first_fit+pages_needed*PAGE_SIZE;
		if(LIST_FIRST(&allocator_list)==NULL){
			LIST_INSERT_HEAD(&allocator_list,block);
		}
		else{
			LIST_INSERT_TAIL(&allocator_list,block);
		}
		return (void*)first_fit;
	}
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy
return NULL;
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");
	uint32 address=(uint32)virtual_address;
	if(address>=USER_HEAP_START && address<sys_get_hard_limit() && sys_isUHeapPlacementStrategyFIRSTFIT()){
		user_list_constant=1;
		free_block(virtual_address);
	}
	else if(address>=sys_get_hard_limit() && address<USER_HEAP_MAX && sys_isUHeapPlacementStrategyFIRSTFIT()){
		uint32 end_address;
		bool found=0;
		struct page_block *block;
		//cprintf("list size is :%lu\n",LIST_SIZE(&allocator_list));
	LIST_FOREACH(block , &allocator_list){
		if(block->lower_bound==address){
			end_address=block->upper_bound;
			LIST_REMOVE(&allocator_list,block);
			user_list_constant=1;
			found=1;
			free_block(block);}
	}

	sys_free_user_mem(address,end_address-address);
}
	else{
		panic("USER FREE PANIC!\n");
	}

}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
