#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

struct free_spaces
{
	uint32 startaddress;
    unsigned int size;
};

int freeindex=-1;
int allocindex=-1;

struct free_spaces freespaces[50000]={0};
struct free_spaces allocatedspaces[50000]={0};

void initilize()
{
	freeindex=-1;
	freeindex++;
	uint32 startadd=KERNEL_HEAP_START;
	uint32 endadd=KERNEL_HEAP_MAX;

	while(startadd<endadd)
	{
		struct Frame_Info *frame=NULL;
		uint32 *ptr_table=NULL;

		frame=get_frame_info(ptr_page_directory,(void *)startadd,&ptr_table);

		  if(frame==NULL)
		  {
		     freespaces[freeindex].startaddress=startadd;
		     freespaces[freeindex].size=PAGE_SIZE;

		     uint32 j=startadd+PAGE_SIZE;
		     for(;j<KERNEL_HEAP_MAX;j+=PAGE_SIZE)
		     {
			    frame=get_frame_info(ptr_page_directory,(void *)j,&ptr_table);
			    if(frame==NULL)
				   freespaces[freeindex].size+=PAGE_SIZE;
			     else
			     {
				   startadd=j+PAGE_SIZE;
				   freeindex++;
				   break;
			     }
		     }
		     if(j==KERNEL_HEAP_MAX)
		    	 break;
		  }

	     else
	     {
	        startadd+=PAGE_SIZE;
	     }
	}
}

void* kmalloc(unsigned int size)
{
	if(freeindex==-1)
	{
		freeindex++;
		initilize();
	}

	struct free_spaces *ptr_best=NULL;
	struct free_spaces *ptr_first=NULL;
	if(size<=0)
		return NULL;

	unsigned int num_pages=ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	uint32 virtualadd;

	if(isKHeapPlacementStrategyBESTFIT())
	{
		int i=0;
			while(i<=freeindex)
			{
				if(freespaces[i].size==size)
				{
					if(ptr_first==NULL)
					{
						ptr_first=&freespaces[i];
					}
					ptr_best=&freespaces[i];
					break;
				}

				if(freespaces[i].size>size)
				{
					if(ptr_first==NULL)
				    {
						ptr_first=&freespaces[i];
					}
					if(ptr_best==NULL)
						ptr_best=&freespaces[i];
					else
					{
						if(ptr_best->size>freespaces[i].size)
						{
							ptr_best=&freespaces[i];
						}
					}
				}
				i++;
			}

		if(ptr_best==NULL)
				return NULL;
	}
	else if(isKHeapPlacementStrategyFIRSTFIT())
	{
		int i=0;
	while(i<=freeindex)
	{
		if(freespaces[i].size>=size)
		{
			ptr_best=&freespaces[i];
			break;
		}
		i++;
	}

		if(ptr_best==NULL)
				return NULL;
	}


  	virtualadd=(uint32)ptr_best->startaddress;
  	allocindex++;
	allocatedspaces[allocindex].startaddress=virtualadd;
    uint32 returnadd=virtualadd;

    for(int j=0;j<num_pages;j++)
    {
    	struct Frame_Info *newframe=NULL;
    	int ret=allocate_frame(&newframe);
    	if(ret==E_NO_MEM)
    		return NULL;

    	ret=map_frame(ptr_page_directory,newframe,(void *)virtualadd,PERM_WRITEABLE|PERM_PRESENT);

    	if(ret==E_NO_MEM)
    	{
    		free_frame(newframe);
    		return NULL;
    	}
    	ptr_best->size-=PAGE_SIZE;
    	ptr_best->startaddress+=PAGE_SIZE;

   		allocatedspaces[allocindex].size+=PAGE_SIZE;
    	virtualadd+=PAGE_SIZE;
    }


    return (void *)returnadd;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2019 - MS1 - [1] Kernel Heap] kfree()
	// Write your code here, remove the panic and write your code
    int found=0;
	for(int i=0;i<=allocindex;i++)
	{
		if((void *)allocatedspaces[i].startaddress==virtual_address)
		{
			found=1;
			unsigned int size=allocatedspaces[i].size;
			unsigned int num_pages=ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
			int j=0;
			while(j<num_pages)
			{
			    unmap_frame(ptr_page_directory,virtual_address);
			    allocatedspaces[i].size-=PAGE_SIZE;
			    virtual_address+=PAGE_SIZE;
				j++;
			}
		    allocatedspaces[i].startaddress=0;
			break;
		}
	}

	if(found==1)
	{
	  initilize();
	}
	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2019 - MS1 - [1] Kernel Heap] kheap_virtual_address()
	// Write your code here, remove the panic and write your code

	struct Frame_Info *frame=NULL;

	frame=to_frame_info(physical_address);
    int16 offset=physical_address%PAGE_SIZE;

    for(int i=0;i<=allocindex;i++)
    {
    	int num_pages=ROUNDUP(allocatedspaces[i].size,PAGE_SIZE)/PAGE_SIZE;
    	uint32 startadd=allocatedspaces[i].startaddress;
    	for(int j=0;j<num_pages;j++)
    	{
    	   struct Frame_Info *vframe=NULL;
    	   uint32 *ptr_table=NULL;
    	   vframe=get_frame_info(ptr_page_directory,(void *)startadd,&ptr_table);
    	   if(vframe==frame)
    	   {
    		  return startadd+offset;
    	   }
    	   startadd+=PAGE_SIZE;
         }
    }
    return (unsigned int)NULL;

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details
	//change this "return" according to your answer
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2019 - MS1 - [1] Kernel Heap] kheap_physical_address()
	// Write your code here, remove the panic and write your code
	uint32 *virtualadd=(uint32 *)virtual_address;
	uint32 *ptr_table=NULL;

	get_page_table(ptr_page_directory,(void *)virtualadd,&ptr_table);

	if(ptr_table==NULL)
		return (unsigned int)NULL;
	uint32 entry=ptr_table[PTX(virtual_address)];
	uint32 framenum=entry>>12;

	unsigned int physicaladd=framenum*PAGE_SIZE;

	return physicaladd;

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//change this "return" according to your answer

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
	//TODO: [PROJECT 2019 - BONUS2] Kernel Heap Realloc
	// Write your code here, remove the panic and write your code
  panic("krealloc not implemented ");

}
