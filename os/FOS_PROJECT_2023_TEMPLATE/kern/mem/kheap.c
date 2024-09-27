#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
LIST_HEAD(page_list, page_block);
struct page_list allocator_list;
struct page_block
{
	uint32 lower_bound;
	uint32 upper_bound;
	LIST_ENTRY(page_block) prev_next_info;	/* linked list links */
};
void show_block_allocator_info(){
	uint32 allocated_size=0;
	uint32 free_size=0;
	uint32 allocated_blocks_count=0;
	uint32 iterated_blocks=0;
	uint32 free_blocks_count=0;
	struct BlockMetaData *block;
	LIST_FOREACH(block,&kernel_block_list){
		if(!block->is_free){
			allocated_size+=block->size;
			allocated_blocks_count+=1;
		}
		else if(block->is_free){
			free_size+=block->size;
			free_blocks_count+=1;
		}
		iterated_blocks+=1;
	}
	cprintf("\n----------------------------------------------------------------------------------\n");
	cprintf("Allocated Block Count:%lu  & their size :%lu\n",allocated_blocks_count,allocated_size);
	cprintf("Free Block count:%lu & their size :%lu\n",free_blocks_count,free_size);
	cprintf("total iterated blocks:%lu\n",iterated_blocks);
	cprintf("\n----------------------------------------------------------------------------------\n");

}
uint32 physical_to_virtual_map[65536];
#define page_block_size() (sizeof(struct page_block))
uint32 map_allocate_chunk_pages(uint32 lower_bounder,uint32 upper_bounder,bool useAllocateList){
	uint32 moving_address=lower_bounder;
	uint32 pages_mapped=0;
	while(moving_address<upper_bounder){
		uint32 *page_table;
		struct FrameInfo *frame;
		allocate_frame(&frame);
		uint32 phyiscal_address=to_physical_address(frame);
		uint32 frame_number=phyiscal_address>>12;
		physical_to_virtual_map[frame_number]=moving_address;
		map_frame(ptr_page_directory,frame,moving_address,PERM_WRITEABLE);
		pages_mapped+=1;
		moving_address+=PAGE_SIZE;
	}
	if(useAllocateList){
		kernel_list_constant=1;
		struct page_block *block=(struct page_block*)alloc_block_FF(page_block_size());
		block->lower_bound=lower_bounder;
		block->upper_bound=upper_bounder;
		LIST_INSERT_HEAD(&allocator_list,block);
	}

	return pages_mapped;
}
void initialize_physical_to_virtual_map(){
	for (uint32 i=0;i<number_of_frames-1;i++){
		physical_to_virtual_map[i]=0;
	}
}
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
uint32 count_pages(uint32 lower_bound,uint32 upper_bound,bool free_allocated_constant){
	uint32 pages=0;
	uint32 moving_address=lower_bound;
	while(moving_address<upper_bound){
		uint32 *page_table;
		struct FrameInfo *frame=get_frame_info(ptr_page_directory,moving_address,&page_table);
		if(frame==NULL)
		{
			if(free_allocated_constant)//1 for free
				pages+=1;
		}
		else if(frame!=NULL){
			if(!free_allocated_constant)//0 for allocated
				pages+=1;
		}


		moving_address+=PAGE_SIZE;
	}
	return pages;
}
uint32 search_for_first_fit(uint32 lower_bound,uint32 upper_bound,uint32 needed_pages){
	uint32 free_pages_found=0;
	uint32 moving_address=lower_bound;
	while(moving_address<upper_bound){
		uint32 *page_table;
		struct FrameInfo *frame=get_frame_info(ptr_page_directory,moving_address,&page_table);
		if(frame==NULL){
			free_pages_found+=1;
		}
		else if(frame!=NULL){
			free_pages_found=0;
		}
		if(free_pages_found==needed_pages){
			uint32 ret_address=moving_address-PAGE_SIZE*(needed_pages-1);
			return ret_address;//success
		}
		moving_address+=PAGE_SIZE;
	}
	return 0;//failure
}
uint32 get_first_twelve_bits(uint32 value){
	 // Create a mask with the first eleven bits set to 1 and the rest set to 0.
	    int mask = (1 << 12) - 1;

	    // Use bitwise AND to extract the first eleven bits.
	    uint32 result = value & mask;

	    return result;
}
uint32 free_pages(uint32 lower_bound,uint32 upper_bound){
	uint32 pages_freed=0;
	uint32 moving_address=lower_bound;
	while(moving_address<upper_bound){
		uint32 *page_table;
					struct FrameInfo *frame=get_frame_info(ptr_page_directory,moving_address,&page_table);

					uint32 physical_address=to_physical_address(frame);
					uint32 frame_number=physical_address>>12;
					physical_to_virtual_map[frame_number]=0;
					//frame->references-=1;
					//page_table[PTX(moving_address)]=page_table[PTX(moving_address)]&PERM_AVAILABLE;
					//if(frame->references==0)
						//free_frame(frame);
					unmap_frame(ptr_page_directory,moving_address);
					moving_address+=PAGE_SIZE;
					pages_freed+=1;
	}
	return pages_freed;

}
uint32 deallocate_pages_in_range(uint32 lower_bound,uint32 upper_bound){
	uint32 pages_mapped=0;
	uint32 low_bound=ROUNDUP(lower_bound,PAGE_SIZE);
	uint32 upp_bound=ROUNDUP(upper_bound,PAGE_SIZE);
	uint32 deallocated_pages=free_pages(low_bound,upp_bound);
	return deallocated_pages;
}
uint32 remove_blocks_after_segment_break_kernel(uint32 updated_break){

	uint32 removed_blocks=0;
	struct BlockMetaData *block;
	LIST_FOREACH(block,&kernel_block_list){
		if((uint32)block>=updated_break){
			LIST_REMOVE(&kernel_block_list,block);
		removed_blocks+=1;
		}

	}

	struct BlockMetaData*temp_block;
	LIST_FOREACH(temp_block,&kernel_block_list){
		uint32 address=(uint32)temp_block;
		uint32 address_plus_offset=address+temp_block->size;
		if(address_plus_offset>updated_break && address<updated_break){

			uint32 residual_size=updated_break-address;
			if(residual_size>=sizeOfMetaData())
				{
				//cprintf("a block has been resized in sbrk\n");
				temp_block->size=residual_size;

				temp_block->is_free=1;
				}

			else if(residual_size<sizeOfMetaData()){

				LIST_PREV(temp_block)->size+=residual_size;
				LIST_REMOVE(&kernel_block_list,temp_block);
				removed_blocks+=1;
			}
		}
	}

	return removed_blocks;
}
int map_allocate_Additional_pages(struct page_block* currblk,uint32 pages_to_be_allocated){
	uint32 moving_address=currblk->upper_bound;
	uint32 new_upper_bound = currblk->upper_bound +(pages_to_be_allocated * PAGE_SIZE);
	uint32 *page_table;
	while(moving_address<new_upper_bound){
		struct FrameInfo *frame=get_frame_info(ptr_page_directory,moving_address,&page_table);
		if(frame==NULL)
		{
			//basel
			allocate_frame(&frame);
			uint32 phyiscal_address=to_physical_address(frame);
			uint32 frame_number=phyiscal_address>>12;
			physical_to_virtual_map[frame_number]=moving_address;
			map_frame(ptr_page_directory,frame,moving_address,PERM_WRITEABLE);
			moving_address+=PAGE_SIZE;
		}
		else
		{
			return -1;
		}
	}
	currblk->upper_bound = new_upper_bound;
	return 1;
}
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...
	//panic("not implemented yet");
	LIST_INIT(&allocator_list);

	//cprintf("frames number is %lu\n",number_of_frames);
	block_allocator_start=daStart;
	//cprintf("block allocator start :%x\n",block_allocator_start);
	segment_break=daStart+initSizeToAllocate;
	segment_break=ROUNDUP(segment_break,PAGE_SIZE);
	aligned_break=segment_break;
	hard_limit=daLimit;
	//cprintf("break before update:%x\n",segment_break);
	if(segment_break>hard_limit){
		panic("NO MEM IN KHEAP INITIALIZER!!\n");
		return E_NO_MEM;
	}
	initialize_physical_to_virtual_map();
	uint32 pages=map_allocate_chunk_pages(block_allocator_start,segment_break,0);
			//cprintf("pages mapped :%lu\n",pages);
	initialize_dynamic_allocator(daStart,initSizeToAllocate);

	return 0;
}








void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING====

	//panic("not implemented yet");
	//cprintf("sbrk has been called...\n");
	if(increment==0){
		//cprintf("0 increment...\n");
		return (void*)segment_break;
	}
	else if(increment>0){
		//cprintf("positive increment...\n");
		uint32 mapped_free_size=aligned_break-segment_break;
		if(mapped_free_size>=increment){
			uint32 new_brk=aligned_break;
			uint32 old_brk=segment_break;
			segment_break=aligned_break;
			//cprintf("no extra pages have been mapped in sbrk...\n");
			return (void*)old_brk;
		}
		else if(mapped_free_size<increment){
			uint32 needed_size=increment-mapped_free_size;
			uint32 pages=needed_pages_for_size(needed_size);
			uint32 new_brk=aligned_break+pages*PAGE_SIZE;
			if(new_brk>hard_limit){
				//cprintf("hard limit has been passed!!!\n");
				return (void*)-1;
			}
			uint32 allocated_pages=map_allocate_chunk_pages(aligned_break,new_brk,0);
			uint32 old_brk=segment_break;
			segment_break=new_brk;
			aligned_break=segment_break;
			//cprintf("pages allocated in sbrk %lu\n",allocated_pages);
			return (void*)old_brk;

		}
	}

	else if(increment<0){
		int abs_increment=-1*increment;
		uint32 new_brk=segment_break-abs_increment;
		if(new_brk<block_allocator_start)
			new_brk=block_allocator_start;
		uint32 removed_blocks=remove_blocks_after_segment_break_kernel(new_brk);
		uint32 pages=deallocate_pages_in_range(new_brk,segment_break);
		//cprintf("blocks removed in sbrk:%lu\n",removed_blocks);
		//cprintf("pages deallocated in sbrk:%lu\n",pages);
		segment_break=new_brk;
		aligned_break=ROUNDUP(segment_break,PAGE_SIZE);
		return (void*)segment_break;
	}
	return NULL;
}


void* kmalloc(unsigned int size)
{	uint32 memory_pages=needed_pages_for_size(KERNEL_HEAP_MAX-hard_limit-PAGE_SIZE);
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy

	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	//cprintf("break : %x\n",segment_break);


	if(size<=DYN_ALLOC_MAX_BLOCK_SIZE && isKHeapPlacementStrategyFIRSTFIT()){
		//cprintf("block allocator from kmalloc \n");
		kernel_list_constant=1;
		return alloc_block_FF(size);
	}
	else if (size>DYN_ALLOC_MAX_BLOCK_SIZE && isKHeapPlacementStrategyFIRSTFIT()){
		//cprintf("page allocator from kmalloc\n");
		// count allocated pages...
		uint32 allocated_pages=count_pages(hard_limit+PAGE_SIZE,KERNEL_HEAP_MAX,0);
		//count needed pages...
		uint32 needed_pages=needed_pages_for_size(size);
		if(allocated_pages+needed_pages>memory_pages)
			return NULL;
		//search for first fit...
		uint32 first_fit=search_for_first_fit(hard_limit+PAGE_SIZE,KERNEL_HEAP_MAX,needed_pages);
		if(first_fit==0){
			cprintf("kmalloc failure returned zero...\n");
			return NULL;
		}
		map_allocate_chunk_pages(first_fit,first_fit+PAGE_SIZE*needed_pages,1);




		return (void*)first_fit;
	}

	return NULL;




}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");
	uint32 address=(uint32)virtual_address;

	if(address>=block_allocator_start && address<=segment_break){
		kernel_list_constant=1;
		//cprintf("free block from kfree!\n");
		free_block(virtual_address);
	}
	else if(address>=hard_limit+PAGE_SIZE && address<KERNEL_HEAP_MAX){
		bool found=0;
		//address=ROUNDDOWN(address,PAGE_SIZE);
		//cprintf("free pages from kfree!\n");
		uint32 end_address=0;
		//cprintf("address is %x\n",address);
		struct page_block *block;
		LIST_FOREACH(block , &allocator_list){
			if(block->lower_bound==address){
				end_address=block->upper_bound;
				found=1;
				LIST_REMOVE(&allocator_list,block);
				free_block(block);
			}
		}
		//if(!found)
		//cprintf("address not found!!\n");
		//else
		//cprintf("address found!!\n");

		free_pages(address,end_address);



	}// end else if
	else{
		panic("invalid address!!!\n");
	}
	}//end function



unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	//if(physical_address>=1*1024*1024 && physical_address<=100*1024*1024){
		//return 0;
	//}
	if(physical_address>>12==0){
		return 0;
	}
	uint32 frame_number=physical_address>>12;
	uint32 offset=get_first_twelve_bits(physical_address);
	uint32 page_address=physical_to_virtual_map[frame_number];
	uint32 *page_table;
	int ret = get_page_table(ptr_page_directory,page_address,&page_table);
	if(ret==TABLE_IN_MEMORY){
		uint32 table_entry=page_table[PTX(page_address)];
		uint32 virtual_frame_number=table_entry>>12;
		uint32 physical_frame_number=table_entry>>12;
		if(physical_frame_number==virtual_frame_number){
			//cprintf("virtual address :%x\n",page_address+offset);
			return page_address+offset;
		}
	}
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer
	uint32 offset=get_first_twelve_bits(virtual_address);
	uint32 *page_table;
	int ret=get_page_table(ptr_page_directory,virtual_address,&page_table);
	if(ret==TABLE_NOT_EXIST)
		return 0;
	uint32 table_entry=page_table[PTX(virtual_address)];
	uint32 frame_number=table_entry>>12;
	uint32 physical_address=frame_number*PAGE_SIZE+offset;
	return physical_address;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{

	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	if(new_size ==0 ){
		kfree(virtual_address);
	}
	else if(virtual_address ==NULL)
	{
		kmalloc(new_size);
	}
	else if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE && isKHeapPlacementStrategyFIRSTFIT())
	{
		kernel_list_constant = 1;
		realloc_block_FF(virtual_address,new_size);
	}
	else if(new_size >DYN_ALLOC_MAX_BLOCK_SIZE&& isKHeapPlacementStrategyFIRSTFIT())
	{
		uint32 address = (uint32)virtual_address;
		struct page_block*currblk;
		LIST_FOREACH(currblk,&allocator_list)
		{
			if(currblk->lower_bound == address)
			{
				uint32 current_blk_size = currblk->upper_bound - currblk->lower_bound;
				uint32 the_new_blk_size = new_size;
				the_new_blk_size = ROUNDUP(the_new_blk_size,PAGE_SIZE);
				if(the_new_blk_size >current_blk_size)
				{
					uint32 the_additional_size = the_new_blk_size - current_blk_size;
					uint32 extra_pages_needed = needed_pages_for_size(the_additional_size);
					uint32 memory_pages=needed_pages_for_size(KERNEL_HEAP_MAX-hard_limit-PAGE_SIZE);
					uint32 allocated_pages = count_pages(hard_limit+PAGE_SIZE,KERNEL_HEAP_MAX,0);
					if(extra_pages_needed + allocated_pages > memory_pages)
						return NULL;
					int ret = map_allocate_Additional_pages(currblk,extra_pages_needed);
					if(ret == -1)
					{
						void*space_found = kmalloc(new_size);
						if(space_found == NULL)
						{
							return NULL;
						}
						else
						{
							void*va = (void*)currblk->lower_bound;
							kfree(va);
							return (void*)space_found;
						}
					}
					else
					{
						return (void*)currblk->lower_bound;
					}
				}
				else if(the_new_blk_size < current_blk_size)
				{
					uint32 deallocated_size = current_blk_size - the_new_blk_size ;
					uint32 pages_to_remove = needed_pages_for_size(deallocated_size);
					uint32 moving_address = currblk->upper_bound - PAGE_SIZE;
					uint32 *page_table;
					for(int i = 0;i < pages_to_remove;i++)
					{
						//basel
						struct FrameInfo *frame=get_frame_info(ptr_page_directory,moving_address,&page_table);
						uint32 phyiscal_address=to_physical_address(frame);
						uint32 frame_number=phyiscal_address>>12;
						physical_to_virtual_map[frame_number]= 0 ;
						unmap_frame(ptr_page_directory,moving_address);
						moving_address-= PAGE_SIZE;
					}
					currblk->upper_bound = moving_address +PAGE_SIZE;
					return (void*)currblk->lower_bound;
				}
				else if(the_new_blk_size == current_blk_size)
				{
					return (void*)currblk->lower_bound;
				}
			}
		}

	}

	return NULL;
}
