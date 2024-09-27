/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
struct MemBlock_LIST free_blocks;
struct MemBlock_LIST allocated_blocks;

bool prev_adjacent(struct BlockMetaData* block,struct BlockMetaData *prev_block){
	bool ret=0;
	uint32 prev_address=(uint32)prev_block;
	uint32 block_address=(uint32)block;
	if(prev_address+prev_block->size==block_address)
		ret=1;
	return ret;
}
bool next_adjacent(struct BlockMetaData*block,struct BlockMetaData*next_block){
	bool ret=0;
	uint32 next_address=(uint32)next_block;
	uint32 block_address=(uint32)block;
	if(block_address+block->size==next_address)
		ret=1;
	return ret;
}











uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	//=========================================
	//=========================================

	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//panic("initialize_dynamic_allocator is not implemented yet");

struct BlockMetaData *initial_block=(struct BlockMetaData*)daStart;
initial_block->size=initSizeOfAllocatedSpace;
initial_block->is_free=1;
LIST_INIT(&free_blocks);
LIST_INIT(&allocated_blocks);
LIST_INSERT_HEAD(&free_blocks,initial_block);

//cprintf("the first pointer size is:%lu\n",LIST_FIRST(&MetaBlock_List)->size);
//cprintf("the first pointer free is:%i\n",is_free_block(LIST_FIRST(&MetaBlock_List)));
//print_blocks_list(free_blocks);
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{		//print_blocks_list(MetaBlock_List);
	uint32 pure_size= size;
	uint32 meta_size=sizeOfMetaData();
	//print_blocks_list(free_blocks);
	if(pure_size==0)
		return NULL;
	//we must always perform a linear search in order to find the first fit address
	uint32 min_address=0;
	bool first_time_flag=1;
	struct BlockMetaData*block;
LIST_FOREACH(block,&free_blocks){
	if(size<=block->size-meta_size && first_time_flag){
		min_address=(uint32)block;
		first_time_flag=0;
	}
	if(size<=block->size-meta_size)
		if((uint32)block<min_address)
			min_address=(uint32)block;
}
if(min_address>0){
	struct BlockMetaData* first_fit=(struct BlockMetaData*)min_address;
	uint32 remaining_size=first_fit->size-meta_size-pure_size;
	if(remaining_size>=meta_size){
		uint32 address=min_address+pure_size+meta_size;
		struct BlockMetaData*block=(struct BlockMetaData*)address;
		block->is_free=1;
		block->size=remaining_size;
		first_fit->size=first_fit->size-remaining_size;
		first_fit->is_free=0;
		LIST_INSERT_TAIL(&free_blocks,block);
		LIST_REMOVE(&free_blocks,first_fit);
		LIST_INSERT_TAIL(&allocated_blocks,first_fit);
		return (void*)(first_fit+1);
	}
	else{
		first_fit->is_free=0;
		LIST_REMOVE(&free_blocks,first_fit);
		LIST_INSERT_TAIL(&allocated_blocks,first_fit);
		return (void*)(first_fit+1);
	}
}



	int ret=(int)sbrk(pure_size+meta_size);
	if(ret==-1)
		return NULL;
	uint32 lower_bound=(uint32)ret;
	uint32 upper_bound=(uint32)sbrk(0);
	uint32 address=lower_bound+pure_size+meta_size;
	uint32 gained_size=upper_bound-lower_bound;
	uint32 remaining_size=gained_size-pure_size-meta_size;
	if(remaining_size>=meta_size){
		struct BlockMetaData*block1=(struct BlockMetaData*)lower_bound;
		struct BlockMetaData*block2=(struct BlockMetaData*)address;
		block1->size=pure_size+meta_size;
		block1->is_free=0;
		block2->size=remaining_size;
		block2->is_free=1;
		LIST_INSERT_TAIL(&allocated_blocks,block1);
		LIST_INSERT_TAIL(&free_blocks,block2);
		return (void*)(block1+1);
	}
	else{
		struct BlockMetaData *block=(struct BlockMetaData*)lower_bound;
		block->size=gained_size;
		block->is_free=0;
		LIST_INSERT_TAIL(&allocated_blocks,block);
		return (void*)(block+1);
	}

	cprintf("all conditions failed returned null!\n");
return NULL;
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	panic("alloc_block_BF is not implemented yet");
	return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================




void free_block(void *va)
{
    //TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");
	if(va==NULL)
		return;
	struct BlockMetaData*block=((struct BlockMetaData*)va-1);
	if(block->is_free==0){
		block->is_free=1;
		LIST_REMOVE(&allocated_blocks,block);
		LIST_INSERT_TAIL(&free_blocks,block);
	}
	struct BlockMetaData*itr_block=NULL;
	LIST_FOREACH(itr_block,&free_blocks){
		if(itr_block!=NULL && prev_adjacent(block,itr_block)){
			itr_block->size+=block->size;
			block->size=0;
			block->is_free=0;
			LIST_REMOVE(&free_blocks,block);
			return free_block((void*)(itr_block+1));
		}
		if(itr_block!=NULL && next_adjacent(block,itr_block)){
			block->size+=itr_block->size;
			itr_block->size=0;
			itr_block->is_free=0;
			LIST_REMOVE(&free_blocks,itr_block);
			return free_block((void*)(block+1));
		}
	}

}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");

	if(va ==NULL && new_size == 0)
	{
		return alloc_block_FF(0);
	}
    else if(va ==NULL)
	{
		struct BlockMetaData* blk =  alloc_block_FF(new_size);
		return blk;
	}
	else if(new_size == 0)
	{
		free_block(va);
		return NULL;
	}
	else
	{
		struct BlockMetaData* currblk = (struct BlockMetaData*)va - 1;
		uint32 next_address = (uint32)currblk + currblk->size;
		struct BlockMetaData *next_blk=(struct BlockMetaData*)next_address;

		uint32 Newsize_withoutMetaData = new_size;
		uint32 curr_blk_size = currblk->size - sizeOfMetaData();

		if(Newsize_withoutMetaData > curr_blk_size&&next_blk->is_free==1)
		{
			if(Newsize_withoutMetaData +sizeOfMetaData() <= currblk->size + next_blk->size)
			{
				next_blk->is_free =0;
				next_blk->size =0;
				LIST_REMOVE(&free_blocks,next_blk);
				currblk->size = Newsize_withoutMetaData +sizeOfMetaData();
				return currblk+1;
			}
		}
		else if(Newsize_withoutMetaData > curr_blk_size&&next_blk->is_free==0)
		{
			return alloc_block_FF(Newsize_withoutMetaData);
		}
		else if(Newsize_withoutMetaData < curr_blk_size)
		{
			uint32 remaining_size = curr_blk_size - Newsize_withoutMetaData;
			if(remaining_size >= sizeOfMetaData())
			{
				char *point=(char*)(currblk)+sizeOfMetaData()+Newsize_withoutMetaData;
				struct BlockMetaData *created_block=(struct BlockMetaData*)point;
				currblk->size =  sizeOfMetaData() + Newsize_withoutMetaData;
				created_block->size = remaining_size;
				created_block->is_free = 1;
				LIST_INSERT_TAIL(&free_blocks,created_block);
			}
			else
			{
				currblk->size =  sizeOfMetaData() + Newsize_withoutMetaData;
			}
			return currblk+1;
		}
		else if(Newsize_withoutMetaData==curr_blk_size)
		{
			return currblk+1;
		}

	}
	return NULL;
}
